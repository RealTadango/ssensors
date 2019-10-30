#ifndef SPORT_H
    #define SPORT_H

    #define SPORT_BAUD 57600
    #define SPORT_START 0x7E
    #define SPORT_HEADER_DISCARD 0x00
    #define SPORT_HEADER_DATA 0x10

    #include <Arduino.h>
    #include <SoftwareSerial.h>

    typedef union {
        char byteValue[4];
        long longValue;
    } longHelper;

    struct sensorData {
        long value;
        int sensorId;
    };

    class SPortSensor {
        public:
            void (*valueSend)(void);
            virtual sensorData getData () = 0;
    };

    class CustomSPortSensor : public SPortSensor {
        public:
            CustomSPortSensor(sensorData (*callback)(CustomSPortSensor*));
            virtual sensorData getData ();
        private:
            sensorData (*_callback)(CustomSPortSensor*);
    };

    class SimpleSPortSensor : public SPortSensor {
        public: 
            SimpleSPortSensor(int id);
            virtual sensorData getData ();
            long value;
        private:
            int _id;
    };

    class SPortHub {
        public:
    #ifdef Serial_
            SPortHub(int physicalId, Serial_& serial);
    #else
            SPortHub(int physicalId, HardwareSerial& serial);
    #endif
            SPortHub(int physicalId, int softwarePin);
            void begin();
            void handle();
            void registerSensor(SPortSensor& sensor);
            void (*commandReceived)(int prim, int applicationId, int value);
            int commandId;
        private:
            void SendData(sensorData data);
            void SendByte(byte b);
            void SendSensor();
            byte GetChecksum(byte data[], int start, int len);

    #ifdef Serial_
            Serial_* _hwStream;
    #else
            HardwareSerial* _hwStream;
    #endif
            SoftwareSerial* _swStream;
            SPortSensor** _sensors;
            int _sensorCount;
            int _sensorIndex;
            int _physicalId;
            int _softwarePin;
            bool _valid;
            short _index;
            byte _buffer[10];
    };

#endif