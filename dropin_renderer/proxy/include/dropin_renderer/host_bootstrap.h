#ifndef DROPIN_HOST_BOOTSTRAP_H
#define DROPIN_HOST_BOOTSTRAP_H

#include "hercules_shim.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Process-attach result. Rejection leaves DirectDraw forwarding available.
 * Query and backend selection happen after loading, on the host UI thread.
 * An installed clone and its DLL must remain loaded for process lifetime. */
typedef enum HsBootstrapStatus {
    HS_BOOTSTRAP_UNCHECKED = 0,
    HS_BOOTSTRAP_INSTALLED = 1,
    HS_BOOTSTRAP_DISABLED = 2,
    HS_BOOTSTRAP_UNSUPPORTED_HOST = 3,
    HS_BOOTSTRAP_TABLE_MISMATCH = 4,
    HS_BOOTSTRAP_INSTALL_FAILED = 5
} HsBootstrapStatus;

HS_API HsBootstrapStatus HS_CALL hs_bootstrap_status(void);

/* Internal composition entry; called only by the forwarder's DllMain. */
void hs_bootstrap_attach(void);

#ifdef __cplusplus
}
#endif
#endif
