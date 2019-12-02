#include "Sockets.h"

#include "Util.h"

#include <stdlib.h>
#include <stdbool.h>

Sockets Sockets_Init(const int32_t port)
{
    IPaddress ip;
    SDLNet_ResolveHost(&ip, NULL, port);
    static Sockets zero;
    Sockets sockets = zero;
    sockets.self = SDLNet_TCP_Open(&ip);
    sockets.set = SDLNet_AllocSocketSet(COLOR_COUNT);
    return sockets;
}

void Sockets_Free(const Sockets sockets)
{
    SDLNet_TCP_Close(sockets.self);
    SDLNet_FreeSocketSet(sockets.set);
}

static Sockets Add(Sockets sockets, TCPsocket socket)
{
    for(int32_t i = 0; i < COLOR_COUNT; i++)
        if(sockets.socket[i] == NULL)
        {
            SDLNet_TCP_AddSocket(sockets.set, socket);
            sockets.socket[i] = socket;
            return sockets;
        }
    return sockets;
}

Sockets Sockets_Service(Sockets sockets, const int32_t timeout)
{
    if(SDLNet_CheckSockets(sockets.set, timeout))
        for(int32_t i = 0; i < COLOR_COUNT; i++)
        {
            TCPsocket socket = sockets.socket[i];
            if(SDLNet_SocketReady(socket))
            {
                static Overview zero;
                Overview overview = zero;
                const int32_t max = sizeof(overview);
                const int32_t bytes = SDLNet_TCP_Recv(socket, &overview, max);
                if(bytes <= 0)
                {
                    SDLNet_TCP_DelSocket(sockets.set, socket);
                    sockets.cycles[i] = 0;
                    sockets.parity[i] = 0;
                    sockets.socket[i] = NULL;
                }
                if(bytes == max)
                {
                    sockets.cycles[i] = overview.cycles;
                    sockets.parity[i] = overview.parity;
                    if(Overview_UsedAction(overview))
                        sockets.packet.overview[i] = overview;
                }
            }
        }
    return sockets;
}

static Sockets Clear(Sockets sockets)
{
    static Packet zero;
    sockets.packet = zero;
    return sockets;
}

static int32_t GetCycleSetpoint(const Sockets sockets)
{
    int32_t setpoint = 0;
    int32_t count = 0;
    for(int32_t i = 0; i < COLOR_COUNT; i++)
    {
        const int32_t cycles = sockets.cycles[i];
        if(cycles > 0)
        {
            setpoint += cycles;
            count++;
        }
    }
    return (count > 0) ? (setpoint / count) : 0;
}

static Sockets CalculateControlChars(Sockets sockets, const int32_t setpoint)
{
    for(int32_t i = 0; i < COLOR_COUNT; i++)
    {
        const int32_t cycles = sockets.cycles[i];
        if(cycles > setpoint)
            sockets.control[i] = PACKET_CONTROL_SLOW_DOWN;
        if(cycles == setpoint)
            sockets.control[i] = PACKET_CONTROL_STEADY;
        if(cycles < setpoint)
            sockets.control[i] = PACKET_CONTROL_SPEED_UP;
    }
    return sockets;
}

static void Print(const Sockets sockets, const int32_t setpoint)
{
    printf("%d :: %d\n", sockets.turn, setpoint);
    for(int32_t i = 0; i < COLOR_COUNT; i++)
    {
        const uint64_t parity = sockets.parity[i];
        const int32_t cycles = sockets.cycles[i];
        const char control = sockets.control[i];
        printf("%d :: 0x%016lX :: %c :: %d\n", i, parity, control, cycles);
    }
}

static void Send(const Sockets sockets, const int32_t setpoint)
{
    for(int32_t i = 0; i < COLOR_COUNT; i++)
    {
        TCPsocket socket = sockets.socket[i];
        if(socket)
        {
            Packet packet = sockets.packet;
            packet.control = sockets.control[i];
            packet.turn = sockets.turn;
            packet.exec_cycle = setpoint + 5;
            packet.index = i;
            SDLNet_TCP_Send(socket, &packet, sizeof(packet));
        }
    }
}

static bool ShouldRelay(const int32_t cycles, const int32_t interval)
{
    return (cycles % interval) == 0;
}

Sockets Sockets_Relay(Sockets sockets, const int32_t cycles, const int32_t interval)
{
    if(ShouldRelay(cycles, interval))
    {
        const int32_t setpoint = GetCycleSetpoint(sockets);
        sockets = CalculateControlChars(sockets, setpoint);
        Print(sockets, setpoint);
        Send(sockets, setpoint);
        sockets.turn++;
        return Clear(sockets);
    }
    return sockets;
}

Sockets Sockets_Accept(const Sockets sockets)
{
    const TCPsocket client = SDLNet_TCP_Accept(sockets.self);
    return (client != NULL)
        ? Add(sockets, client)
        : sockets;
}
