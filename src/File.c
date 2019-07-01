#include "File.h"

#include "Util.h"

File File_Load(FILE* const fp)
{
    static File zero;
    File file = zero;
    UTIL_FREAD(&file.id, 1, fp);
    UTIL_FREAD(&file.data_offset, 1, fp);
    UTIL_FREAD(&file.size, 1, fp);
    return file;
}

void File_Print(const File file)
{
    Util_Log("\t\t\tid          : %d\n", file.id);
    Util_Log("\t\t\tdata offset : %d\n", file.data_offset);
    Util_Log("\t\t\tsize        : %d\n", file.size);
}
