#ifndef PUBLISHER_H
#define PUBLISHER_H

// Minimal publishing abstraction used by consumers that report data
// (Interface Segregation: only what a reporter needs).
class IPublisher {
public:
  virtual ~IPublisher() = default;

  virtual bool isConnected() const = 0;
  virtual bool publish(const char* topic, const char* payload) = 0;
};

#endif  // PUBLISHER_H
