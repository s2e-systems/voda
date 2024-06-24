use dust_dds::{
    domain::domain_participant_factory::DomainParticipantFactory,
    infrastructure::{error::DdsError, qos::QosKind, status::NO_STATUS},
};
use gstreamer::prelude::*;

#[derive(Debug, dust_dds::topic_definition::type_support::DdsType)]
struct Video<'a> {
    user_id: i16,
    frame_num: i32,
    frame: &'a [u8],
}
#[derive(Debug)]
struct Error(String);
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.0)
    }
}
impl From<gstreamer::glib::Error> for Error {
    fn from(value: gstreamer::glib::Error) -> Self {
        Self(format!("GStreamer error: {}", value))
    }
}
impl From<gstreamer::StateChangeError> for Error {
    fn from(value: gstreamer::StateChangeError) -> Self {
        Self(format!("GStreamer state change error: {}", value))
    }
}
impl From<&gstreamer::message::Error> for Error {
    fn from(value: &gstreamer::message::Error) -> Self {
        Self(format!("GStreamer state change error: {:?}", value))
    }
}
impl From<DdsError> for Error {
    fn from(value: DdsError) -> Self {
        Self(format!("DDS error: {:?}", value))
    }
}

fn main() -> Result<(), Error> {
    gstreamer::init()?;

    let domain_id = 0;
    let participant_factory = DomainParticipantFactory::get_instance();
    let participant =
        participant_factory.create_participant(domain_id, QosKind::Default, None, NO_STATUS)?;
    let topic = participant.create_topic::<Video>(
        "VideoStream",
        "Video",
        QosKind::Default,
        None,
        NO_STATUS,
    )?;
    let publisher = participant.create_publisher(QosKind::Default, None, NO_STATUS)?;
    let writer = publisher.create_datawriter(&topic, QosKind::Default, None, NO_STATUS)?;

    let pipeline = gstreamer::parse::launch(
        r#"autovideosrc ! video/x-raw,framerate=[1/1,25/1],width=[1,1280],height=[1,720] ! tee name=t ! queue leaky=2 ! videoconvert ! openh264enc complexity=0 scene-change-detection=0 background-detection=0 bitrate=1280000 ! appsink name=appsink sync=false t. ! queue leaky=2 ! taginject tags="title=Publisher" ! autovideosink"#,
    )?;

    pipeline.set_state(gstreamer::State::Playing)?;

    let bin = pipeline
        .downcast_ref::<gstreamer::Bin>()
        .expect("Pipeline must be bin");
    let appsink_element = bin.by_name("appsink").expect("appsink in pipeline");
    let appsink = appsink_element
        .downcast_ref::<gstreamer_app::AppSink>()
        .expect("type AppSink");

    let mut i = 0;
    appsink.set_callbacks(
        gstreamer_app::AppSinkCallbacks::builder()
            .new_sample(move |s| {
                if let Ok(sample) = s.pull_sample() {
                    let bytes = sample
                        .buffer()
                        .expect("buffer exists")
                        .map_readable()
                        .expect("readable buffer");

                    let video_sample = Video {
                        user_id: 8,
                        frame_num: i,
                        frame: bytes.as_slice(),
                    };
                    writer
                        .write(&video_sample, None)
                        .expect("Sample could not be written");

                    i += 1;
                    println!("Wrote sample {:?}", i);
                    use std::io::{self, Write};
                    io::stdout().flush().ok();
                }

                Ok(gstreamer::FlowSuccess::Ok)
            })
            .build(),
    );

    // Wait until error or EOS
    let bus = pipeline.bus().expect("pipeline has bus");
    for msg in bus.iter_timed(gstreamer::ClockTime::NONE) {
        match msg.view() {
            gstreamer::MessageView::Eos(..) => break,
            gstreamer::MessageView::Error(err) => Err(err)?,
            _ => (),
        }
    }

    pipeline.set_state(gstreamer::State::Null)?;

    Ok(())
}
