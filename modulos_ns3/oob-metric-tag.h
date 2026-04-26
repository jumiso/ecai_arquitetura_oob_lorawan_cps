
#ifndef OOB_METRIC_TAG_H
#define OOB_METRIC_TAG_H

#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {
namespace lorawan {

class OobMetricTag : public Tag
{
public:
  OobMetricTag () : m_value (1) {}

  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::lorawan::OobMetricTag")
      .SetParent<Tag> ()
      .AddConstructor<OobMetricTag> ();
    return tid;
  }

  TypeId GetInstanceTypeId (void) const override
  {
    return GetTypeId ();
  }

  uint32_t GetSerializedSize (void) const override
  {
    return 1;
  }

  void Serialize (TagBuffer i) const override
  {
    i.WriteU8 (m_value);
  }

  void Deserialize (TagBuffer i) override
  {
    m_value = i.ReadU8 ();
  }

  void Print (std::ostream& os) const override
  {
    os << "oob=" << (uint32_t)m_value;
  }

  void SetValue (uint8_t v) { m_value = v; }
  uint8_t GetValue () const { return m_value; }

private:
  uint8_t m_value;
};

} // namespace lorawan
} // namespace ns3

#endif 
// Arquivo criado para simulacao da Arquitetura Out-of-Band baseada em LoraWan para Sistemas Ciberfisicos Industriais - https://github.com/jumiso/ecai_arquitetura_oob_lorawan_cps/
