#include <Arduino.h>
#include <SoftwareSerial.h>
#include "SPort.h"

#ifdef Serial_
SPortHub::SPortHub(int physicalId, Serial_& serial):
  _physicalId(physicalId),
  _hwStream(&serial),
  _sensorIndex(0) {
}
#else
SPortHub::SPortHub(int physicalId, HardwareSerial& serial):
  _physicalId(physicalId),
  _hwStream(&serial),
  _sensorIndex(0) {
}
#endif

SPortHub::SPortHub(int physicalId, int softwarePin):
  _physicalId(physicalId),
  _softwarePin(softwarePin),
  _sensorIndex(0) {
    _swStream = new SoftwareSerial(_softwarePin, _softwarePin, true);
}

void SPortHub::begin() {
    if(_hwStream) {
      _hwStream->begin(SPORT_BAUD, SERIAL_8E2);
    } else {
      _swStream->begin(SPORT_BAUD);
    }
}

void SPortHub::SendSensor() {
  if (_sensorIndex >= _sensorCount) {
    _sensorIndex = 0;
  } 

  SPortSensor *sensor = _sensors[_sensorIndex];
  sensorData data = sensor->getData();
  SendData(data);
  if(sensor->valueSend) {
    sensor->valueSend();
  }

  _sensorIndex++;
}

void SPortHub::handle() {
    Stream* stream = _hwStream ? (Stream*)_hwStream : (Stream*)_swStream;

    while(stream->available() > 0) {
        byte newByte = stream->read();

        if(newByte == SPORT_START) {
          _valid = true;
          _index = 0;
        } else if(_valid && _index == 1) {
          //Check if the frame / request is for us
          int physicalID = newByte & 0x1F;

          if(_physicalId == physicalID && stream->available() == 0) {
            SendSensor();
            _valid = false;
          } else if(commandId != physicalID) {
            //Other ID or to late
            _valid = false;
          }
        }

        if(_valid) {
          _buffer[_index] = newByte;
          _index++;

          if(_index >= 10)
          {
            _sensorIndex = 0;
            _valid = false;
            if(commandReceived) {
              int applicationId = _buffer[3] + (_buffer[4] * 256);
              longHelper lh;
              lh.byteValue[0] = _buffer[5];
              lh.byteValue[1] = _buffer[6];
              lh.byteValue[2] = _buffer[7];
              lh.byteValue[3] = _buffer[8];
              commandReceived(_buffer[2], applicationId, lh.longValue);
            }
          }
        }
    }
}

void SPortHub::registerSensor(SPortSensor& sensor) {
    SPortSensor** newSensors = new SPortSensor*[_sensorCount + 1];

    for(int i = 0; i < _sensorCount; i++) {
      newSensors[i] = _sensors[i];
    }
    newSensors[_sensorCount] = &sensor;
    _sensors = newSensors;
    _sensorCount++;
}

void SPortHub::SendData(sensorData data) {
    if(_swStream) {
        pinMode(_softwarePin, OUTPUT);
        delay(1);
    }

    byte frame[8];

    if(data.sensorId > 0) {
        longHelper lh;
        lh.longValue = data.value;
        frame[0] = SPORT_HEADER_DATA;    
        frame[1] = lowByte(data.sensorId);
        frame[2] = highByte(data.sensorId);
        frame[3] = lh.byteValue[0];
        frame[4] = lh.byteValue[1];
        frame[5] = lh.byteValue[2];
        frame[6] = lh.byteValue[3];
    } else {
        frame[0] = SPORT_HEADER_DISCARD;    
        frame[1] = 0;
        frame[2] = 0;
        frame[3] = 0;
        frame[4] = 0;
        frame[5] = 0;
        frame[6] = 0;
    }

    frame[7] = GetChecksum(frame, 0, 7);

    //Send the frame
    for(short i = 0; i < 8; i++) {
        SendByte(frame[i]);
    }

    if(_swStream) {
        pinMode(_softwarePin, INPUT);
    }
}

byte SPortHub::GetChecksum(byte data[], int start, int len) {
  long total = 0;

  for(int i = start; i < (start + len); i++) {
    total += data[i];
  }

  if(total >= 0x700) {
    total+= 7;
  } else if(total >= 0x600) {
    total+= 6;
  } else if(total >= 0x500) {
    total+= 5;
  } else if(total >= 0x400) {
    total+= 4;
  } else if(total >= 0x300) {
    total+= 3;
  } else if(total >= 0x200) {
    total+= 2;
  } else if(total >= 0x100) {
    total++;
  }

  return 0xFF - total;
}

//Send a data byte the FrSky way
void SPortHub::SendByte(byte b) {
  Stream* stream = _hwStream ? (Stream*)_hwStream : (Stream*)_swStream;

  if(b == 0x7E) {
    stream->write(0x7D);
    stream->write(0x5E);
  } else if(b == 0x7D) {
    stream->write(0x7D);
    stream->write(0x5D);
  } else {
    stream->write(b);
  }
}

CustomSPortSensor::CustomSPortSensor(sensorData (*callback)(CustomSPortSensor*)) {
  _callback = callback;
}

sensorData CustomSPortSensor::getData() {
  return _callback(this);
}

SimpleSPortSensor::SimpleSPortSensor(int id) {
  _id = id;
  value = 0;
}

sensorData SimpleSPortSensor::getData() {
    sensorData data;
    data.sensorId = _id;
    data.value = value;
    return data;
}