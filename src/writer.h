#ifndef WRITER_H
#define WRITER_H

class Writer
{
private:
  int pin;
  bool status = false;

public:
  Writer(int pin)
  {
    this->pin = pin;
  }

  void setup()
  {
    pinMode(this->pin, OUTPUT);
    this->off();
  }

  bool getStatus () {
    return this->status;
  }

  void on()
  {
    this->status = true;
    digitalWrite(this->pin, HIGH);
  }

  void off()
  {
    this->status = false;
    digitalWrite(this->pin, LOW);
  }
};

#endif // WRITER_H