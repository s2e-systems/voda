use dust_dds::{
    domain::domain_participant_factory::DomainParticipantFactory,
    infrastructure::{qos::QosKind, status::NO_STATUS},
};
use gstreamer::{self, prelude::*, DebugCategory, DebugLevel, DebugMessage};
use gstreamer_video_sys::GstVideoOverlay;
use jni::{
    objects::{GlobalRef, JClass, JObject, JValueGen},
    sys::jint,
    JNIEnv, JavaVM,
};
use ndk_sys::android_LogPriority;
use std::ffi::CString;

struct VodaError(String);

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

static mut GSTREAMER_VIDEO_PIPELINE: Option<gstreamer::Pipeline> = None;

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

    let tag = format!("GStreamer+{}", category.name());
    match object {
        Some(obj) => {
            let label = obj.to_string();
            let msg = format!(
                "{} {}:{}:{}:{} {}",
                gstreamer::get_timestamp(),
                file,
                line,
                function,
                label,
                message.get().unwrap()
            );
            android_log_write(prio, &tag, &msg);
        }
        None => {
            let msg = format!(
                "{} {}:{}:{} {}",
                gstreamer::get_timestamp(),
                file,
                line,
                function,
                message.get().unwrap()
            );
            android_log_write(prio, &tag, &msg);
        }
    }
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

/// Sets the surface to the GStreamer video system
/// # Safety
/// Must use the ndk and the global instance of the gstreamer pipeline
#[no_mangle]
unsafe extern "C" fn Java_com_s2e_1systems_SurfaceHolderCallback_nativeSurfaceInit(
    env: JNIEnv,
    _: JClass,
    surface: jni::sys::jobject,
) {
    if let Some(pipeline) = GSTREAMER_VIDEO_PIPELINE.as_ref() {
        let overlay = pipeline.by_interface(gstreamer_video::VideoOverlay::static_type());
        if let Some(overlay) = &overlay {
            let overlay = overlay.as_ptr() as *mut GstVideoOverlay;
            let native_window = ndk_sys::ANativeWindow_fromSurface(env.get_raw(), surface);
            gstreamer_video_sys::gst_video_overlay_set_window_handle(
                overlay,
                native_window as usize,
            )
        }
    } else {
        android_log_write(
            android_LogPriority::ANDROID_LOG_ERROR,
            "VoDA",
            "Pipeline not initialized yet",
        );
    }
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
    ndk_sys::ANativeWindow_release(ndk_sys::ANativeWindow_fromSurface(env.get_raw(), surface));
}

/// Stores the Java class loader, adds log handlers to glib and GStreamer, initializes GStreamer and registers GStreamer plugins
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
                            env.exception_describe().unwrap();
                            env.exception_clear().unwrap();
                            return;
                        }
                    }
                    Err(e) => {
                        android_log_write(
                            android_LogPriority::ANDROID_LOG_ERROR,
                            "VoDA",
                            &format!("{}", e),
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
        Err(e) => {
            android_log_write(
                android_LogPriority::ANDROID_LOG_ERROR,
                "VoDA",
                &format!("{}", e),
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
    }

    gst_plugin_opengl_register();
    gst_plugin_app_register();
    gst_plugin_coreelements_register();
    gst_plugin_openh264_register();
    gst_plugin_videoconvertscale_register();
    gst_plugin_androidmedia_register();
}

/// Creates the GStreamer pipeline and stores it as a global
/// # Safety
/// Must initialize the global GStreamer pipeline
#[no_mangle]
unsafe extern "C" fn Java_com_s2e_1systems_MainActivity_nativeRun(_env: JNIEnv, _: JClass) {
    match create_pipeline() {
        Ok(pipeline) => {
            GSTREAMER_VIDEO_PIPELINE = Some(pipeline);
            std::thread::spawn(move || {
                main_loop(GSTREAMER_VIDEO_PIPELINE.as_ref().expect("Pipeline is some"))
                    .unwrap_or_else(|e| {
                        android_log_write(
                            android_LogPriority::ANDROID_LOG_ERROR,
                            "VoDA",
                            &e.to_string(),
                        )
                    })
            });
        }
        Err(err) => android_log_write(
            android_LogPriority::ANDROID_LOG_ERROR,
            "VoDA",
            &format!("Creating pipeline failed with: {}", err),
        ),
    };
}

fn create_pipeline() -> Result<gstreamer::Pipeline, VodaError> {
    let pipeline_element = gstreamer::parse::launch("ahcsrc ! video/x-raw,framerate=[1/1,25/1],width=[1,1280],height=[1,720] ! tee name=t ! queue leaky=2 max-size-buffers=1 ! glimagesink t. ! queue leaky=2 max-size-buffers=1 ! videoconvert ! openh264enc complexity=0 scene-change-detection=0 background-detection=0 bitrate=1280000 ! appsink name=appsink max-buffers=1 sync=false")?;

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
    let sink = pipeline
        .by_name("appsink")
        .ok_or(VodaError("appsink missing".to_string()))?;
    let appsink = sink
        .dynamic_cast::<gstreamer_app::AppSink>()
        .expect("Sink element is expected to be an appsink!");

    let mut i = 0;
    appsink.set_callbacks(
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

    Ok(pipeline)
}

fn main_loop(pipeline: &gstreamer::Pipeline) -> Result<(), VodaError> {
    pipeline.set_state(gstreamer::State::Playing)?;

    let bus = pipeline
        .bus()
        .expect("Pipeline without bus. Shouldn't happen!");

    for msg in bus.iter_timed(gstreamer::ClockTime::NONE) {
        match msg.view() {
            gstreamer::MessageView::Eos(..) => break,
            gstreamer::MessageView::Error(err) => {
                pipeline.set_state(gstreamer::State::Null)?;
                Err(err)?;
            }
            _ => (),
        }
    }
    pipeline.set_state(gstreamer::State::Null)?;

    Ok(())
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
