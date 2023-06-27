#include "general.h"
#include "gdb_if.h"
#include "gdb_main.h"
#include "target.h"
#include "exception.h"
#include "spi.h"

int main(void)
{
    platform_init();

    while (true) {
        volatile struct exception e;
        TRY_CATCH(e, EXCEPTION_ALL) {
            gdb_main();
        }
        if (e.type) {
            disable_supply();
            target_lost();
        }
    }

    /* Should never get here */
    return 0;
}