#ifndef BLINKER_PROTOCOL_H
#define BLINKER_PROTOCOL_H

template <class Transp>
class BlinkerProtocol
{
    public :
        template<class Transp>
        void adapters(Transp & transp);

    private :
        Transp&     conn;
};

#endif