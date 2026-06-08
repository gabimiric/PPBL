#ifndef SENSOR_H
#define SENSOR_H

#include "Module.h"

// ============================================================================
// BASE CLASS FOR SENSORS
// ============================================================================

class Sensor : public Module
{
public:
    virtual ~Sensor() {}

    /**
     * Get the last read sensor value.
     * Return type and units depend on specific sensor implementation.
     */
    virtual float getValue() const = 0;

    /**
     * Get sensor reading timestamp (milliseconds since startup).
     */
    virtual unsigned long getLastReadTime() const = 0;

    /**
     * Check if reading is fresh (within update interval).
     */
    virtual bool isFresh() const = 0;

protected:
    float _value = 0.0f;
    unsigned long _lastReadTime = 0;
};

#endif // SENSOR_H
