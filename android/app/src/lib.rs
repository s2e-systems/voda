use dust_dds::{
    domain::domain_participant_factory::DomainParticipantFactory,
    infrastructure::{
        qos::QosKind,
        status::{StatusKind, NO_STATUS},
    },
    subscription::{
        data_reader_listener::DataReaderListener,
        sample_info::{ANY_INSTANCE_STATE, ANY_SAMPLE_STATE, ANY_VIEW_STATE},
    },
};
use gstreamer::{self, prelude::*, DebugCategory, DebugLevel, DebugMessage, Pipeline};
use gstreamer_video_sys::GstVideoOverlay;
use jni::{
    objects::{GlobalRef, JClass, JObject, JValueGen},
    sys::jint,
    JNIEnv, JavaVM,
};
use ndk_sys::android_LogPriority;
use std::{ffi::CString, sync::Mutex, thread::JoinHandle};

#[derive(Debug)]
struct VodaError(String);
impl VodaError {
    fn android_log_write(&self) {
        android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "VoDA", &self.0)
    }
}
impl std::fmt::Display for VodaError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl From<glib::Error> for VodaError {
    fn from(value: glib::Error) -> Self {
        VodaError(format!("GLib error: {}", value))
    }
}

impl From<gstreamer::StateChangeError> for VodaError {
    fn from(value: gstreamer::StateChangeError) -> Self {
        VodaError(format!("StateChangeError: {}", value))
    }
}

impl From<&gstreamer::message::Error> for VodaError {
    fn from(err: &gstreamer::message::Error) -> Self {
        Self(format!(
            "GStreamer error: {}, debug: {:?}",
            err.error(),
            err.debug().map(|s| s.to_string())
        ))
    }
}

impl From<dust_dds::infrastructure::error::DdsError> for VodaError {
    fn from(value: dust_dds::infrastructure::error::DdsError) -> Self {
        VodaError(format!("DdsError: {:?}", value))
    }
}

#[derive(Debug, dust_dds::topic_definition::type_support::DdsType)]
struct Video<'a> {
    user_id: i16,
    frame_num: i32,
    frame: &'a [u8],
}

static mut JAVA_VM: Option<JavaVM> = None;
static mut CLASS_LOADER: Option<GlobalRef> = None;
static mut NATIVE_WINDOW: Option<usize> = None;

static APPLICATION: Mutex<Option<Application>> = Mutex::new(None);

/// Convenience function that removes the DDS domain participant from domain 0
fn delete_participant() {
    let factory = DomainParticipantFactory::get_instance();
    if let Ok(Some(participant)) = &factory.lookup_participant(0) {
        if let Err(err) = participant.delete_contained_entities() {
            VodaError::from(err).android_log_write();
        }
        if let Err(err) = factory.delete_participant(participant) {
            VodaError::from(err).android_log_write();
        }
    }
}

struct Publisher {
    pipeline: gstreamer::Pipeline,
    join_handle: Option<JoinHandle<()>>,
}

impl Publisher {
    fn new() -> Result<Self, VodaError> {
        let pipeline_element = gstreamer::parse::launch("ahcsrc ! video/x-raw,framerate=[1/1,25/1],width=[1,1280],height=[1,720] ! videoflip name=video_flip ! tee name=t ! queue leaky=2 max-size-buffers=1 ! glimagesink t. ! queue leaky=2 max-size-buffers=1 ! videoconvert ! openh264enc min-force-key-unit-interval=1000000000 complexity=0 scene-change-detection=0 background-detection=0 bitrate=512000 ! appsink name=app_sink max-buffers=1 sync=false")?;

        let participant = DomainParticipantFactory::get_instance().create_participant(
            0,
            QosKind::Default,
            None,
            NO_STATUS,
        )?;
        let topic = participant.create_topic::<Video>(
            "VideoStream",
            "Video",
            QosKind::Default,
            None,
            NO_STATUS,
        )?;
        let publisher = participant.create_publisher(QosKind::Default, None, NO_STATUS)?;
        let writer = publisher.create_datawriter(&topic, QosKind::Default, None, NO_STATUS)?;
        let pipeline = pipeline_element
            .dynamic_cast::<gstreamer::Pipeline>()
            .expect("Pipeline is expected to be a bin");
        let app_sink_element = pipeline.by_name("app_sink").expect("has element");
        let app_sink = app_sink_element
            .dynamic_cast::<gstreamer_app::AppSink>()
            .expect("is type AppSink");

        let mut i = 0;
        app_sink.set_callbacks(
            gstreamer_app::AppSinkCallbacks::builder()
                .new_sample(move |s| {
                    if let Ok(sample) = s.pull_sample() {
                        let buffer_map = sample
                            .buffer()
                            .expect("buffer exists")
                            .map_readable()
                            .expect("readable buffer");
                        let video_sample = Video {
                            user_id: 8,
                            frame_num: i,
                            frame: buffer_map.as_slice(),
                        };
                        i += 1;
                        if writer.write(&video_sample, None).is_err() {
                            return Err(gstreamer::FlowError::Error);
                        };
                    }
                    Ok(gstreamer::FlowSuccess::Ok)
                })
                .build(),
        );

        pipeline.set_state(gstreamer::State::Playing)?;

        let bus = pipeline.bus().expect("Pipeline has bus");

        let join_handle = std::thread::spawn(move || {
            for msg in bus.iter_timed(gstreamer::ClockTime::NONE) {
                match msg.view() {
                    gstreamer::MessageView::StateChanged(s) => {
                        if s.current() == gstreamer::State::Null
                            || s.pending() == gstreamer::State::Null
                        {
                            break;
                        }
                    }
                    gstreamer::MessageView::Eos(..) => break,
                    gstreamer::MessageView::Error(err) => {
                        VodaError::from(err).android_log_write();
                        break;
                    }
                    _ => (),
                }
            }
        });

        Ok(Self {
            pipeline,
            join_handle: Some(join_handle),
        })
    }
}

impl Drop for Publisher {
    fn drop(&mut self) {
        self.pipeline
            .set_state(gstreamer::State::Null)
            .expect("pipeline settable to Null");
        if let Some(join_handle) = self.join_handle.take() {
            if let Err(_) = join_handle.join() {
                VodaError("publisher join failed".to_string()).android_log_write();
            }
        };
        delete_participant();
    }
}

struct Subscriber {
    pipeline: gstreamer::Pipeline,
    join_handle: Option<JoinHandle<()>>,
}

impl Subscriber {
    fn new() -> Result<Self, VodaError> {
        struct Listener {
            appsrc: gstreamer_app::AppSrc,
        }

        impl<'a> DataReaderListener<'a> for Listener {
            type Foo = Video<'a>;

            fn on_data_available(
                &mut self,
                the_reader: dust_dds::subscription::data_reader::DataReader<Self::Foo>,
            ) {
                if let Ok(samples) =
                    the_reader.read(1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE)
                {
                    for sample in samples {
                        if let Ok(sample_data) = sample.data() {
                            android_log_write(
                                android_LogPriority::ANDROID_LOG_INFO,
                                "VoDA",
                                &format!("sample received: {:?}", sample_data.frame_num),
                            );

                            let mut buffer = gstreamer::Buffer::with_size(sample_data.frame.len())
                                .expect("buffer creation failed");
                            {
                                let buffer_ref = buffer.get_mut().expect("mutable buffer");
                                let mut buffer_samples =
                                    buffer_ref.map_writable().expect("writeable buffer");
                                buffer_samples.clone_from_slice(sample_data.frame);
                            }
                            self.appsrc
                                .push_buffer(buffer)
                                .expect("push buffer into appsrc succeeds");
                        }
                    }
                }
            }
        }
        let pipeline_element = gstreamer::parse::launch(
            "appsrc name=app_src ! openh264dec ! videoconvert ! glimagesink sync=false",
        )?;
        let bin = pipeline_element
            .downcast_ref::<gstreamer::Bin>()
            .expect("Pipeline is bin");
        let appsrc_element = bin.by_name("app_src").expect("Pipeline has appsrc");
        let appsrc = appsrc_element
            .downcast::<gstreamer_app::AppSrc>()
            .expect("is AppSrc type");
        let src_caps = gstreamer::Caps::builder("video/x-h264")
            .field("stream-format", "byte-stream")
            .field("alignment", "au")
            .field("profile", "constrained-baseline")
            .build();
        appsrc.set_caps(Some(&src_caps));

        let factory = DomainParticipantFactory::get_instance();
        let participant = factory.create_participant(0, QosKind::Default, None, NO_STATUS)?;
        let topic = participant.create_topic::<Video>(
            "VideoStream",
            "Video",
            QosKind::Default,
            None,
            NO_STATUS,
        )?;
        let subscriber = participant.create_subscriber(QosKind::Default, None, NO_STATUS)?;
        let _reader = subscriber.create_datareader::<Video>(
            &topic,
            QosKind::Default,
            Some(Box::new(Listener { appsrc })),
            &[StatusKind::DataAvailable],
        )?;

        let pipeline = pipeline_element
            .dynamic_cast::<gstreamer::Pipeline>()
            .expect("Pipeline is a bin");

        pipeline.set_state(gstreamer::State::Playing)?;

        let bus = pipeline.bus().expect("Pipeline has bus");

        let join_handle = std::thread::spawn(move || {
            for msg in bus.iter_timed(gstreamer::ClockTime::NONE) {
                match msg.view() {
                    gstreamer::MessageView::StateChanged(s) => {
                        if s.current() == gstreamer::State::Null
                            || s.pending() == gstreamer::State::Null
                        {
                            break;
                        }
                    }
                    gstreamer::MessageView::Eos(..) => break,
                    gstreamer::MessageView::Error(err) => {
                        VodaError::from(err).android_log_write();
                        break;
                    }
                    _ => (),
                }
            }
        });

        Ok(Subscriber {
            pipeline,
            join_handle: Some(join_handle),
        })
    }
}

impl Drop for Subscriber {
    fn drop(&mut self) {
        self.pipeline
            .set_state(gstreamer::State::Null)
            .expect("pipeline settable to Null");
        if let Some(join_handle) = self.join_handle.take() {
            if let Err(_) = join_handle.join() {
                VodaError("Subscriber join failed".to_string()).android_log_write();
            }
        };
        delete_participant();
    }
}

enum Application {
    Publisher(Publisher),
    Subscriber(Subscriber),
}

fn android_log_write(prio: android_LogPriority, tag: &str, msg: &str) {
    let tag_c = CString::new(tag).expect("tag str not converted to CString");
    let msg_c = CString::new(msg).expect("msg str not converted to CString");
    unsafe {
        ndk_sys::__android_log_write(
            prio.0 as std::os::raw::c_int,
            tag_c.as_ptr(),
            msg_c.as_ptr(),
        );
    }
}

fn glib_print_handler(msg: &str) {
    android_log_write(android_LogPriority::ANDROID_LOG_INFO, "GLib+stdout", msg);
}

fn glib_printerr_handler(msg: &str) {
    android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "GLib+stderr", msg);
}

fn glib_log_handler(domain: Option<&str>, level: glib::LogLevel, msg: &str) {
    let prio = match level {
        glib::LogLevel::Error => android_LogPriority::ANDROID_LOG_ERROR,
        glib::LogLevel::Critical => android_LogPriority::ANDROID_LOG_ERROR,
        glib::LogLevel::Warning => android_LogPriority::ANDROID_LOG_WARN,
        glib::LogLevel::Message => android_LogPriority::ANDROID_LOG_INFO,
        glib::LogLevel::Info => android_LogPriority::ANDROID_LOG_INFO,
        glib::LogLevel::Debug => android_LogPriority::ANDROID_LOG_DEBUG,
    };
    let tag = format!("Glib+{}", domain.unwrap_or(""));
    android_log_write(prio, &tag, msg);
}

fn gstreamer_log_function(
    category: DebugCategory,
    level: DebugLevel,
    file: &gstreamer::glib::GStr,
    function: &gstreamer::glib::GStr,
    line: u32,
    object: Option<&gstreamer::log::LoggedObject>,
    message: &DebugMessage,
) {
    if level > category.threshold() {
        return;
    }
    let prio = match level {
        DebugLevel::Error => android_LogPriority::ANDROID_LOG_ERROR,
        DebugLevel::Warning => android_LogPriority::ANDROID_LOG_WARN,
        DebugLevel::Info => android_LogPriority::ANDROID_LOG_INFO,
        DebugLevel::Debug => android_LogPriority::ANDROID_LOG_DEBUG,
        _ => android_LogPriority::ANDROID_LOG_VERBOSE,
    };
    let ts = gstreamer::get_timestamp();
    let tag = format!("GStreamer+{}", category.name());
    let msg = match object {
        Some(obj) => {
            let label = obj.to_string();
            format!(
                "{} {}:{}:{}:{} {:?}",
                ts, file, line, function, label, message
            )
        }
        None => {
            format!("{} {}:{}:{} {:?}", ts, file, line, function, message)
        }
    };
    android_log_write(prio, &tag, &msg);
}

/// This functions is searched by name by the androidmedia plugin. It must hence be present
/// even if it appears to be unused
/// # Safety
/// Must use global JAVA_VM
#[no_mangle]
unsafe extern "C" fn gst_android_get_java_vm() -> *const jni::sys::JavaVM {
    match JAVA_VM.as_ref() {
        Some(vm) => vm.get_java_vm_pointer(),
        None => std::ptr::null(),
    }
}

/// This functions is searched by name by the androidmedia plugin. It must hence be present
/// even if it appears to be unused
/// # Safety
/// Must use global CLASS_LOADER
#[no_mangle]
unsafe extern "C" fn gst_android_get_application_class_loader() -> jni::sys::jobject {
    match CLASS_LOADER.as_ref() {
        Some(o) => o.as_raw(),
        None => std::ptr::null_mut(),
    }
}

unsafe fn set_window_handle_to_overlay_in_pipeline(pipeline: &Pipeline, native_window: usize) {
    let overlay = pipeline
        .by_interface(gstreamer_video::VideoOverlay::static_type())
        .expect("Pipeline has VideoOverlay");
    gstreamer_video_sys::gst_video_overlay_set_window_handle(
        overlay.as_ptr() as *mut GstVideoOverlay,
        native_window,
    );
}

/// Sets the surface to the GStreamer video system
/// # Safety
/// Must use the ndk and the global instance of the gstreamer pipeline
#[no_mangle]
unsafe extern "C" fn Java_com_s2e_1systems_MainActivity_nativeRotationChanged(
    _env: JNIEnv,
    _: JClass,
    rotation: jni::sys::jint,
) {
    if let Ok(application) = APPLICATION.lock() {
        if let Some(Application::Publisher(p)) = application.as_ref() {
            let video_direction = match rotation {
                0 => gstreamer_video::VideoOrientationMethod::_90r,
                1 => gstreamer_video::VideoOrientationMethod::Identity,
                3 => gstreamer_video::VideoOrientationMethod::_180,
                _ => gstreamer_video::VideoOrientationMethod::Identity,
            };

            match p.pipeline.by_name("video_flip") {
                Some(videoflip) => videoflip
                    .set_property_from_value("video-direction", &video_direction.to_value()),
                None => VodaError("videoflip not present".to_string()).android_log_write(),
            }
        }
    }
}

/// Sets the surface to the GStreamer video system
/// # Safety
/// Must use the ndk and the global instance of the gstreamer pipeline
#[no_mangle]
unsafe extern "C" fn Java_com_s2e_1systems_SurfaceHolderCallback_nativeSurfaceInit(
    env: JNIEnv,
    _: JClass,
    surface: jni::sys::jobject,
) {
    let native_window = ndk_sys::ANativeWindow_fromSurface(env.get_raw(), surface) as usize;
    if let Ok(application_lock) = APPLICATION.lock() {
        if let Some(application) = application_lock.as_ref() {
            let pipeline = match application {
                Application::Publisher(p) => &p.pipeline,
                Application::Subscriber(s) => &s.pipeline,
            };
            set_window_handle_to_overlay_in_pipeline(pipeline, native_window);
        }
    }
    NATIVE_WINDOW = Some(native_window);
}

/// Releases the surface
/// # Safety
/// Must use the NDK
#[no_mangle]
unsafe extern "C" fn Java_com_s2e_1systems_SurfaceHolderCallback_nativeSurfaceFinalize(
    env: JNIEnv,
    _: JClass,
    surface: jni::sys::jobject,
) {
    VodaError("nativeSurfaceFinalize called".to_string()).android_log_write();
    ndk_sys::ANativeWindow_release(ndk_sys::ANativeWindow_fromSurface(env.get_raw(), surface));
}

/// Stores the Java class loader, adds log handlers to glib and GStreamer, initializes GStreamer and registers GStreamer plugins
/// This is called via the provided GStreamer.java template.
/// # Safety
/// Must instantiate CLASS_LOADER global and make use of the NDK
#[no_mangle]
unsafe extern "C" fn Java_org_freedesktop_gstreamer_GStreamer_nativeInit(
    mut env: JNIEnv,
    _: JClass,
    context: JObject,
) {
    // Store class loader
    match env.call_method(&context, "getClassLoader", "()Ljava/lang/ClassLoader;", &[]) {
        Ok(loader) => match loader {
            JValueGen::Object(obj) => {
                CLASS_LOADER = env.new_global_ref(obj).ok();
                match env.exception_check() {
                    Ok(value) => {
                        if value {
                            env.exception_describe().expect("exception describable");
                            env.exception_clear().expect("exception clearable");
                            return;
                        }
                    }
                    Err(err) => {
                        android_log_write(
                            android_LogPriority::ANDROID_LOG_ERROR,
                            "VoDA",
                            &format!("{}", err),
                        );
                        return;
                    }
                }
            }
            _ => {
                android_log_write(
                    android_LogPriority::ANDROID_LOG_ERROR,
                    "VoDA",
                    "Could not get class loader",
                );
                return;
            }
        },
        Err(err) => {
            android_log_write(
                android_LogPriority::ANDROID_LOG_ERROR,
                "VoDA",
                &format!("{}", err),
            );
            return;
        }
    }

    glib::set_print_handler(glib_print_handler);
    glib::set_printerr_handler(glib_printerr_handler);
    glib::log_set_default_handler(glib_log_handler);

    gstreamer::log::set_active(true);
    gstreamer::log::set_default_threshold(gstreamer::DebugLevel::Warning);
    gstreamer::log::remove_default_log_function();
    gstreamer::log::add_log_function(gstreamer_log_function);

    match gstreamer::init() {
        Ok(_) => { /* Do nothing. */ }
        Err(e) => {
            android_log_write(
                android_LogPriority::ANDROID_LOG_ERROR,
                "VoDA",
                &format!("GStreamer initialization failed: {}", e),
            );
            match env.find_class("java/lang/Exception") {
                Ok(c) => {
                    env.throw_new(c, &format!("GStreamer initialization failed: {}", e))
                        .ok();
                }
                Err(e) => {
                    android_log_write(
                        android_LogPriority::ANDROID_LOG_ERROR,
                        "VoDA",
                        &format!("Could not get Exception class: {}", e),
                    );
                    return;
                }
            }
            return;
        }
    }

    extern "C" {
        fn gst_plugin_opengl_register();
        fn gst_plugin_app_register();
        fn gst_plugin_coreelements_register();
        fn gst_plugin_openh264_register();
        fn gst_plugin_videoconvertscale_register();
        fn gst_plugin_androidmedia_register();
        fn gst_plugin_videofilter_register();
    }

    gst_plugin_opengl_register();
    gst_plugin_app_register();
    gst_plugin_coreelements_register();
    gst_plugin_openh264_register();
    gst_plugin_videoconvertscale_register();
    gst_plugin_androidmedia_register();
    gst_plugin_videofilter_register();
}

/// Creates the GStreamer publisher pipeline and stores it as a global
/// # Safety
/// Must initialize the global APPLICATION
#[no_mangle]
unsafe extern "C" fn Java_com_s2e_1systems_MainActivity_nativeRunPublisher(
    _env: JNIEnv,
    _: JClass,
) {
    if let Ok(mut application_lock) = APPLICATION.lock() {
        *application_lock = None;
        match Publisher::new() {
            Ok(publisher) => {
                if let Some(native_window) = NATIVE_WINDOW {
                    set_window_handle_to_overlay_in_pipeline(&publisher.pipeline, native_window);
                }
                *application_lock = Some(Application::Publisher(publisher));
            }
            Err(err) => {
                err.android_log_write();
            }
        };
    }
}

/// Creates the GStreamer subscriber pipeline and stores it as a global
/// # Safety
/// Must initialize the global GStreamer pipeline
#[no_mangle]
unsafe extern "C" fn Java_com_s2e_1systems_MainActivity_nativeRunSubscriber(
    _env: JNIEnv,
    _: JClass,
) {
    if let Ok(mut application_lock) = APPLICATION.lock() {
        *application_lock = None;
        match Subscriber::new() {
            Ok(subscriber) => {
                if let Some(native_window) = NATIVE_WINDOW {
                    set_window_handle_to_overlay_in_pipeline(&subscriber.pipeline, native_window);
                }
                *application_lock = Some(Application::Subscriber(subscriber));
            }
            Err(err) => {
                err.android_log_write();
            }
        }
    }
}

/// Store Java VM
/// # Safety
/// Must use global JAVA_VM
#[no_mangle]
#[allow(non_snake_case)]
unsafe fn JNI_OnLoad(jvm: JavaVM, _reserved: *mut std::os::raw::c_void) -> jint {
    let mut env: JNIEnv;
    match jvm.get_env() {
        Ok(v) => {
            env = v;
        }
        Err(e) => {
            android_log_write(
                android_LogPriority::ANDROID_LOG_ERROR,
                "VoDA",
                &format!("Could not retrieve JNIEnv, error: {}", e),
            );
            return 0;
        }
    }

    let version: jint;
    match env.get_version() {
        Ok(v) => {
            version = v.into();
            android_log_write(
                android_LogPriority::ANDROID_LOG_INFO,
                "VoDA",
                &format!("JNI Version: {:#x?}", version),
            );
        }
        Err(e) => {
            android_log_write(
                android_LogPriority::ANDROID_LOG_ERROR,
                "VoDA",
                &format!("Could not retrieve JNI version, error: {}", e),
            );
            return 0;
        }
    }

    match env.find_class("org/freedesktop/gstreamer/GStreamer") {
        Ok(_) => {}
        Err(e) => {
            android_log_write(
                android_LogPriority::ANDROID_LOG_ERROR,
                "VoDA",
                &format!(
                    "Could not find class org.freedesktop.gstreamer.GStreamer, error: {}",
                    e
                ),
            );
            return 0;
        }
    }
    JAVA_VM = Some(jvm);
    version
}
