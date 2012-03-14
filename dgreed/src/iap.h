#ifndef IAP_H
#define IAP_H

// iOS In-App Purchase wrapper
// Does not work on non-iOS builds!

#ifndef TARGET_IOS
#error IAP works only on iOS
#endif

#include "utils.h"
#include "darray.h"

typedef struct {
	const char* id;
	const char* title;
	const char* description;
	const char* price_str;
} Product;

typedef void (*ProductCallback)(DArray* data);
typedef void (*PurchaseCallback)(const char* _id, bool success);

void iap_init(const char* ids, ProductCallback product_cb, PurchaseCallback purchase_cb);
void iap_close(void);
bool iap_is_active(void);

void iap_purchase(const char* _id);

#endif
