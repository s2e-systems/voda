use dust_dds::{
    domain::domain_participant_factory::DomainParticipantFactory,
    infrastructure::{
        error::DdsError,
        qos::{DataReaderQos, QosKind},
        qos_policy::HistoryQosPolicy,
        status::{StatusKind, NO_STATUS},
        time::Duration,
        wait_set::{Condition, WaitSet},
    },
    publication::data_writer::DataWriter,
    subscription::{
        data_reader_listener::DataReaderListener,
        sample_info::{ANY_INSTANCE_STATE, ANY_SAMPLE_STATE, ANY_VIEW_STATE},
    },
};

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
impl From<DdsError> for Error {
    fn from(value: DdsError) -> Self {
        Self(format!("DDS error: {:?}", value))
    }
}

struct Listener {
    writer: DataWriter<Video<'static>>,
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
            if let Some(sample) = samples.last() {
                if let Ok(sample_data) = sample.data() {
                    println!("sample received: {:?}", sample_data.frame_num);

                    self.writer.write(&sample_data, None).unwrap();
                }
            }
        }
    }
}

fn main() -> Result<(), Error> {
    let domain_id_receiver = 1;
    let domain_id_sender = 0;
    let participant_factory = DomainParticipantFactory::get_instance();
    let participant_receiver = participant_factory.create_participant(
        domain_id_receiver,
        QosKind::Default,
        None,
        NO_STATUS,
    )?;
    let participant_sender: dust_dds::domain::domain_participant::DomainParticipant =
        participant_factory.create_participant(
            domain_id_sender,
            QosKind::Default,
            None,
            NO_STATUS,
        )?;
    let topic = participant_receiver.create_topic::<Video>(
        "VideoStream",
        "Video",
        QosKind::Default,
        None,
        NO_STATUS,
    )?;
    let publisher = participant_sender.create_publisher(QosKind::Default, None, NO_STATUS)?;
    let writer = publisher.create_datawriter(&topic, QosKind::Default, None, NO_STATUS)?;

    let subscriber = participant_receiver.create_subscriber(QosKind::Default, None, NO_STATUS)?;

    let reader = subscriber.create_datareader(
        &topic,
        QosKind::Specific(DataReaderQos {
            history: HistoryQosPolicy {
                kind: dust_dds::infrastructure::qos_policy::HistoryQosPolicyKind::KeepLast(1),
            },
            ..Default::default()
        }),
        Some(Box::new(Listener { writer })),
        &[StatusKind::DataAvailable],
    )?;

    let reader_cond = reader.get_statuscondition();
    reader_cond
        .set_enabled_statuses(&[StatusKind::InconsistentTopic])
        .unwrap();
    let mut wait_set = WaitSet::new();
    wait_set
        .attach_condition(Condition::StatusCondition(reader_cond.clone()))
        .unwrap();
    wait_set.wait(Duration::new(600, 0)).unwrap();

    Ok(())
}
