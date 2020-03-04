#include "hal.h"
#include "chprintf.h"

template <SerialDriver *SD, bool CTS = false, bool RTS = false>
class SDPrinter {
    private:
        mutex_t lock;

    public:
        SDPrinter() {
            chMtxObjectInit(&lock);
        }

        inline int operator()(const char *s, ...) {
            va_list args;
            va_start(args, s);
            chMtxLock(&lock);
            int r = chvprintf((BaseSequentialStream *)SD, s, args);
            chMtxUnlock(&lock);
            // sdPut();
            // sdPutI();
            return r;
        }
};

int printToSerial(SerialDriver *SD, CANRxFrame) {
  
}