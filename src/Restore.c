#include "Restore.h"

#include "Util.h"

// XXX. THINK OF A WAY TO MAKE A NICE MACRO FOR THESE SIZEOF OPERATORS.

static Points RecvPath(TCPsocket socket)
{
    Points path;
    SDLNet_TCP_Recv(socket, &path.max, sizeof(path.max));
    SDLNet_TCP_Recv(socket, &path.count, sizeof(path.count));
    path.point = UTIL_ALLOC(Point, path.max);
    SDLNet_TCP_Recv(socket, path.point, sizeof(*path.point) * path.count);
    return path;
}

static void SendPath(TCPsocket socket, const Points path)
{
    SDLNet_TCP_Send(socket, &path.max, sizeof(path.max));
    SDLNet_TCP_Send(socket, &path.count, sizeof(path.count));
    SDLNet_TCP_Send(socket, path.point, sizeof(*path.point) * path.count);
}

Restore Restore_Recv(TCPsocket socket)
{
    static Restore zero;
    Restore restore = zero;
    if(socket)
    {
        SDLNet_TCP_Recv(socket, &restore.count, sizeof(restore.count));
        SDLNet_TCP_Recv(socket, &restore.cycles, sizeof(restore.cycles));
        restore.unit = UTIL_ALLOC(Unit, restore.count);
        SDLNet_TCP_Recv(socket, restore.unit, sizeof(*restore.unit) * restore.count);
        // for(int32_t i = 0; i < restore.count; i++)
        // {
        //     printf("RECV PATH %d / %d\n", i, restore.count);
        //     restore.unit->path = RecvPath(socket);
        // }
    }
    return restore;
}

void Restore_Send(const Restore restore, TCPsocket socket)
{
    if(socket)
    {
        SDLNet_TCP_Send(socket, &restore.count, sizeof(restore.count));
        SDLNet_TCP_Send(socket, &restore.cycles, sizeof(restore.cycles));
        SDLNet_TCP_Send(socket, restore.unit, sizeof(*restore.unit) * restore.count);
        // for(int32_t i = 0; i < restore.count; i++)
        // {
        //     printf("SEND PATH %d / %d\n", i, restore.count);
        //     SendPath(socket, restore.unit[i].path);
        // }
    }
}

void Restore_Free(const Restore restore)
{
    free(restore.unit);
}