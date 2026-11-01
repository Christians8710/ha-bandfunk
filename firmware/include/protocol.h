#pragma once
#include <Arduino.h>

// Future decoders consume bounded durations. No invented CrowdSync commands.
struct Pulse { uint32_t duration_us; bool level; };
class ProtocolDecoder {
 public:
  virtual ~ProtocolDecoder() = default;
  virtual const char* name() const = 0;
  virtual void consume(const Pulse& pulse) = 0;
  virtual bool supportsColor() const { return false; }
};
class UnknownOOK final : public ProtocolDecoder {
 public:
  const char* name() const override { return "nicht_entschluesselt"; }
  void consume(const Pulse&) override {}
};
