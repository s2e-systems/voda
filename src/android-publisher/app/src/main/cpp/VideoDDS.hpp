/****************************************************************

  Generated by Eclipse Cyclone DDS IDL to CXX Translator
  File name: VideoDDS.idl
  Source: VideoDDS.hpp
  Cyclone DDS: v0.11.0

*****************************************************************/
#ifndef DDSCXX_VIDEODDS_HPP
#define DDSCXX_VIDEODDS_HPP

#include <cstdint>
#include <vector>

namespace S2E
{
class Video
{
private:
 int16_t userid_ = 0;
 int32_t frameNum_ = 0;
 std::vector<uint8_t> frame_;

public:
  Video() = default;

  explicit Video(
    int16_t userid,
    int32_t frameNum,
    const std::vector<uint8_t>& frame) :
    userid_(userid),
    frameNum_(frameNum),
    frame_(frame) { }

  int16_t userid() const { return this->userid_; }
  int16_t& userid() { return this->userid_; }
  void userid(int16_t _val_) { this->userid_ = _val_; }
  int32_t frameNum() const { return this->frameNum_; }
  int32_t& frameNum() { return this->frameNum_; }
  void frameNum(int32_t _val_) { this->frameNum_ = _val_; }
  const std::vector<uint8_t>& frame() const { return this->frame_; }
  std::vector<uint8_t>& frame() { return this->frame_; }
  void frame(const std::vector<uint8_t>& _val_) { this->frame_ = _val_; }
  void frame(std::vector<uint8_t>&& _val_) { this->frame_ = _val_; }

  bool operator==(const Video& _other) const
  {
    (void) _other;
    return userid_ == _other.userid_ &&
      frameNum_ == _other.frameNum_ &&
      frame_ == _other.frame_;
  }

  bool operator!=(const Video& _other) const
  {
    return !(*this == _other);
  }

};

}

#include "dds/topic/TopicTraits.hpp"
#include "org/eclipse/cyclonedds/topic/datatopic.hpp"

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace topic {

template <> constexpr const char* TopicTraits<::S2E::Video>::getTypeName()
{
  return "S2E::Video";
}

template <> constexpr bool TopicTraits<::S2E::Video>::isSelfContained()
{
  return false;
}

#ifdef DDSCXX_HAS_TYPE_DISCOVERY
template<> constexpr unsigned int TopicTraits<::S2E::Video>::type_map_blob_sz() { return 302; }
template<> constexpr unsigned int TopicTraits<::S2E::Video>::type_info_blob_sz() { return 100; }
template<> inline const uint8_t * TopicTraits<::S2E::Video>::type_map_blob() {
  static const uint8_t blob[] = {
 0x65,  0x00,  0x00,  0x00,  0x01,  0x00,  0x00,  0x00,  0xf1,  0xfd,  0xf0,  0xa6,  0xb6,  0xfd,  0x49,  0x82, 
 0xac,  0x40,  0xeb,  0xc9,  0xd5,  0x73,  0x67,  0x00,  0x4d,  0x00,  0x00,  0x00,  0xf1,  0x51,  0x01,  0x00, 
 0x01,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x3d,  0x00,  0x00,  0x00,  0x03,  0x00,  0x00,  0x00, 
 0x0b,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x31,  0x00,  0x03,  0xea,  0x8f,  0x53,  0x8c,  0x00, 
 0x0b,  0x00,  0x00,  0x00,  0x01,  0x00,  0x00,  0x00,  0x01,  0x00,  0x04,  0x36,  0xf3,  0x6b,  0x57,  0x00, 
 0x15,  0x00,  0x00,  0x00,  0x02,  0x00,  0x00,  0x00,  0x01,  0x00,  0x81,  0xf3,  0x01,  0x00,  0x00,  0x00, 
 0x00,  0xec,  0x5e,  0x00,  0x02,  0xdc,  0xf3,  0xe3,  0x6e,  0x00,  0x00,  0x00,  0x98,  0x00,  0x00,  0x00, 
 0x01,  0x00,  0x00,  0x00,  0xf2,  0x09,  0x45,  0x8d,  0x9d,  0x7e,  0x12,  0xd0,  0xa3,  0x04,  0xd4,  0xa8, 
 0x4a,  0x38,  0xbc,  0x00,  0x80,  0x00,  0x00,  0x00,  0xf2,  0x51,  0x01,  0x00,  0x13,  0x00,  0x00,  0x00, 
 0x00,  0x00,  0x00,  0x00,  0x0b,  0x00,  0x00,  0x00,  0x53,  0x32,  0x45,  0x3a,  0x3a,  0x56,  0x69,  0x64, 
 0x65,  0x6f,  0x00,  0x00,  0x60,  0x00,  0x00,  0x00,  0x03,  0x00,  0x00,  0x00,  0x15,  0x00,  0x00,  0x00, 
 0x00,  0x00,  0x00,  0x00,  0x31,  0x00,  0x03,  0x00,  0x07,  0x00,  0x00,  0x00,  0x75,  0x73,  0x65,  0x72, 
 0x69,  0x64,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x17,  0x00,  0x00,  0x00,  0x01,  0x00,  0x00,  0x00, 
 0x01,  0x00,  0x04,  0x00,  0x09,  0x00,  0x00,  0x00,  0x66,  0x72,  0x61,  0x6d,  0x65,  0x4e,  0x75,  0x6d, 
 0x00,  0x00,  0x00,  0x00,  0x20,  0x00,  0x00,  0x00,  0x02,  0x00,  0x00,  0x00,  0x01,  0x00,  0x81,  0xf3, 
 0x01,  0x00,  0x00,  0x00,  0x00,  0xec,  0x5e,  0x00,  0x02,  0x00,  0x00,  0x00,  0x06,  0x00,  0x00,  0x00, 
 0x66,  0x72,  0x61,  0x6d,  0x65,  0x00,  0x00,  0x00,  0x22,  0x00,  0x00,  0x00,  0x01,  0x00,  0x00,  0x00, 
 0xf2,  0x09,  0x45,  0x8d,  0x9d,  0x7e,  0x12,  0xd0,  0xa3,  0x04,  0xd4,  0xa8,  0x4a,  0x38,  0xbc,  0xf1, 
 0xfd,  0xf0,  0xa6,  0xb6,  0xfd,  0x49,  0x82,  0xac,  0x40,  0xeb,  0xc9,  0xd5,  0x73,  0x67, };
  return blob;
}
template<> inline const uint8_t * TopicTraits<::S2E::Video>::type_info_blob() {
  static const uint8_t blob[] = {
 0x60,  0x00,  0x00,  0x00,  0x01,  0x10,  0x00,  0x40,  0x28,  0x00,  0x00,  0x00,  0x24,  0x00,  0x00,  0x00, 
 0x14,  0x00,  0x00,  0x00,  0xf1,  0xfd,  0xf0,  0xa6,  0xb6,  0xfd,  0x49,  0x82,  0xac,  0x40,  0xeb,  0xc9, 
 0xd5,  0x73,  0x67,  0x00,  0x51,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x04,  0x00,  0x00,  0x00, 
 0x00,  0x00,  0x00,  0x00,  0x02,  0x10,  0x00,  0x40,  0x28,  0x00,  0x00,  0x00,  0x24,  0x00,  0x00,  0x00, 
 0x14,  0x00,  0x00,  0x00,  0xf2,  0x09,  0x45,  0x8d,  0x9d,  0x7e,  0x12,  0xd0,  0xa3,  0x04,  0xd4,  0xa8, 
 0x4a,  0x38,  0xbc,  0x00,  0x84,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x04,  0x00,  0x00,  0x00, 
 0x00,  0x00,  0x00,  0x00, };
  return blob;
}
#endif //DDSCXX_HAS_TYPE_DISCOVERY

} //namespace topic
} //namespace cyclonedds
} //namespace eclipse
} //namespace org

namespace dds {
namespace topic {

template <>
struct topic_type_name<::S2E::Video>
{
    static std::string value()
    {
      return org::eclipse::cyclonedds::topic::TopicTraits<::S2E::Video>::getTypeName();
    }
};

}
}

REGISTER_TOPIC_TYPE(::S2E::Video)

namespace org{
namespace eclipse{
namespace cyclonedds{
namespace core{
namespace cdr{

template<>
propvec &get_type_props<::S2E::Video>();

template<typename T, std::enable_if_t<std::is_base_of<cdr_stream, T>::value, bool> = true >
bool write(T& streamer, const ::S2E::Video& instance, entity_properties_t *props) {
  (void)instance;
  if (!streamer.start_struct(*props))
    return false;
  auto prop = streamer.first_entity(props);
  while (prop) {
    switch (prop->m_id) {
      case 0:
      if (!streamer.start_member(*prop))
        return false;
      if (!write(streamer, instance.userid()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 1:
      if (!streamer.start_member(*prop))
        return false;
      if (!write(streamer, instance.frameNum()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 2:
      if (!streamer.start_member(*prop))
        return false;
      if (!streamer.start_consecutive(false, true))
        return false;
      {
      uint32_t se_1 = uint32_t(instance.frame().size());
      if (se_1 > 6220800 &&
          streamer.status(serialization_status::write_bound_exceeded))
        return false;
      if (!write(streamer, se_1))
        return false;
      if (se_1 > 0 &&
          !write(streamer, instance.frame()[0], se_1))
        return false;
      }  //end sequence 1
      if (!streamer.finish_consecutive())
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
    }
    prop = streamer.next_entity(prop);
  }
  return streamer.finish_struct(*props);
}

template<typename S, std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >
bool write(S& str, const ::S2E::Video& instance, bool as_key) {
  auto &props = get_type_props<::S2E::Video>();
  str.set_mode(cdr_stream::stream_mode::write, as_key);
  return write(str, instance, props.data()); 
}

template<typename T, std::enable_if_t<std::is_base_of<cdr_stream, T>::value, bool> = true >
bool read(T& streamer, ::S2E::Video& instance, entity_properties_t *props) {
  (void)instance;
  if (!streamer.start_struct(*props))
    return false;
  auto prop = streamer.first_entity(props);
  while (prop) {
    switch (prop->m_id) {
      case 0:
      if (!streamer.start_member(*prop))
        return false;
      if (!read(streamer, instance.userid()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 1:
      if (!streamer.start_member(*prop))
        return false;
      if (!read(streamer, instance.frameNum()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 2:
      if (!streamer.start_member(*prop))
        return false;
      if (!streamer.start_consecutive(false, true))
        return false;
      {
      uint32_t se_1 = uint32_t(instance.frame().size());
      if (se_1 > 6220800 &&
          streamer.status(serialization_status::read_bound_exceeded))
        return false;
      if (!read(streamer, se_1))
        return false;
      instance.frame().resize(se_1);
      if (se_1 > 0 &&
          !read(streamer, instance.frame()[0], se_1))
        return false;
      }  //end sequence 1
      if (!streamer.finish_consecutive())
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
    }
    prop = streamer.next_entity(prop);
  }
  return streamer.finish_struct(*props);
}

template<typename S, std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >
bool read(S& str, ::S2E::Video& instance, bool as_key) {
  auto &props = get_type_props<::S2E::Video>();
  str.set_mode(cdr_stream::stream_mode::read, as_key);
  return read(str, instance, props.data()); 
}

template<typename T, std::enable_if_t<std::is_base_of<cdr_stream, T>::value, bool> = true >
bool move(T& streamer, const ::S2E::Video& instance, entity_properties_t *props) {
  (void)instance;
  if (!streamer.start_struct(*props))
    return false;
  auto prop = streamer.first_entity(props);
  while (prop) {
    switch (prop->m_id) {
      case 0:
      if (!streamer.start_member(*prop))
        return false;
      if (!move(streamer, instance.userid()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 1:
      if (!streamer.start_member(*prop))
        return false;
      if (!move(streamer, instance.frameNum()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 2:
      if (!streamer.start_member(*prop))
        return false;
      if (!streamer.start_consecutive(false, true))
        return false;
      {
      uint32_t se_1 = uint32_t(instance.frame().size());
      if (se_1 > 6220800 &&
          streamer.status(serialization_status::move_bound_exceeded))
        return false;
      if (!move(streamer, se_1))
        return false;
      if (se_1 > 0 &&
          !move(streamer, uint8_t(), se_1))
        return false;
      }  //end sequence 1
      if (!streamer.finish_consecutive())
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
    }
    prop = streamer.next_entity(prop);
  }
  return streamer.finish_struct(*props);
}

template<typename S, std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >
bool move(S& str, const ::S2E::Video& instance, bool as_key) {
  auto &props = get_type_props<::S2E::Video>();
  str.set_mode(cdr_stream::stream_mode::move, as_key);
  return move(str, instance, props.data()); 
}

template<typename T, std::enable_if_t<std::is_base_of<cdr_stream, T>::value, bool> = true >
bool max(T& streamer, const ::S2E::Video& instance, entity_properties_t *props) {
  (void)instance;
  if (!streamer.start_struct(*props))
    return false;
  auto prop = streamer.first_entity(props);
  while (prop) {
    switch (prop->m_id) {
      case 0:
      if (!streamer.start_member(*prop))
        return false;
      if (!max(streamer, instance.userid()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 1:
      if (!streamer.start_member(*prop))
        return false;
      if (!max(streamer, instance.frameNum()))
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
      case 2:
      if (!streamer.start_member(*prop))
        return false;
      if (!streamer.start_consecutive(false, true))
        return false;
      {
      uint32_t se_1 = 6220800;
      if (!max(streamer, se_1))
        return false;
      if (se_1 > 0 &&
          !max(streamer, uint8_t(), se_1))
        return false;
      }  //end sequence 1
      if (!streamer.finish_consecutive())
        return false;
      if (!streamer.finish_member(*prop))
        return false;
      break;
    }
    prop = streamer.next_entity(prop);
  }
  return streamer.finish_struct(*props);
}

template<typename S, std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >
bool max(S& str, const ::S2E::Video& instance, bool as_key) {
  auto &props = get_type_props<::S2E::Video>();
  str.set_mode(cdr_stream::stream_mode::max, as_key);
  return max(str, instance, props.data()); 
}

} //namespace cdr
} //namespace core
} //namespace cyclonedds
} //namespace eclipse
} //namespace org

#endif // DDSCXX_VIDEODDS_HPP
