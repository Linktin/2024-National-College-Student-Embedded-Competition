#ifndef __VOICE_H__
#define __VOICE_H__
#include <gpiod.hpp>
#include <string>
#include <QtCore/QTimer>
class Voice : public QObject
{
    Q_OBJECT
private:
    gpiod::chip gpioChip;
    gpiod::line gpioLine61;//GPIO61
    gpiod::line gpioLine62;//GPIO62
    gpiod::line_request request;
public:
    ~Voice();
    Voice(QObject* parent = nullptr);
    void success_voice();
    void Face_entry_voice();
};

#endif