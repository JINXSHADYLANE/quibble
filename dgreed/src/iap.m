#include "iap.h"

#import <StoreKit/StoreKit.h>
#import "DGreedAppDelegate.h"

#include "memory.h"

static DGreedAppDelegate* app_delegate = nil;
static bool can_make_payments = false;
static NSMutableDictionary* products_dict = NULL;
static NSMutableDictionary* prices_dict = NULL;

static ProductCallback _iap_product_cb = NULL;
static PurchaseCallback _iap_purchase_cb = NULL;

void _set_iap_app_delegate(DGreedAppDelegate* _app_delegate) {
    app_delegate = _app_delegate;
}

static void _iap_get_products(ProductCallback cb) {
    DArray products = darray_create(sizeof(Product), [products_dict count]);
    
    for(id key in products_dict) {
        SKProduct* value = [products_dict objectForKey:key];
        NSString* price = [prices_dict objectForKey:key];
        
        Product pr = {
            .id = [value.productIdentifier UTF8String],
            .title = [value.localizedTitle UTF8String],
            .description = [value.localizedDescription UTF8String],
            .price_str = [price UTF8String]
        };
        
        darray_append(&products, &pr);
    }
    
    (*cb)(&products);
    
    darray_free(&products);
}


void iap_init(const char* ids, ProductCallback product_cb, PurchaseCallback purchase_cb) {
    if ([SKPaymentQueue canMakePayments]) {
        
        products_dict = [[NSMutableDictionary alloc] init];
        prices_dict = [[NSMutableDictionary alloc] init];
        
        _iap_product_cb = product_cb;
        _iap_purchase_cb = purchase_cb;
        
        [[SKPaymentQueue defaultQueue] addTransactionObserver:app_delegate];

		// Split ids
		NSMutableSet* ids_set = [[NSMutableSet alloc] init];

		char* cloned_ids = alloca(strlen(ids)+1);
        strcpy(cloned_ids, ids);
        
		char* _id;

		_id = strtok(cloned_ids, ",");
		while(_id != NULL) {
			[ids_set addObject:[NSString stringWithUTF8String:_id]];
			_id = strtok(NULL, ",");
		}
        
        SKProductsRequest *request= [[[SKProductsRequest alloc] initWithProductIdentifiers:ids_set] autorelease];
        
        [ids_set release];

        request.delegate = app_delegate;
        [request start];
    }
}

void _iap_received_products_response(SKProductsResponse* response) {
    LOG_INFO("Received products response");
        
    NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
    
    for(SKProduct* product in response.invalidProductIdentifiers) {
        LOG_WARNING("Invalid product!");
        can_make_payments = false;
    }
    
    for(SKProduct* product in response.products) {
        [products_dict setValue:product forKey:product.productIdentifier];
        [numberFormatter setLocale:product.priceLocale];
        [prices_dict setValue:[numberFormatter stringFromNumber:product.price] forKey:product.productIdentifier];
        can_make_payments = true;
    }
    
    [numberFormatter release];

    _iap_get_products(_iap_product_cb);
}

void iap_close(void) {
    if(products_dict)
        [products_dict release];
    if(prices_dict)
        [prices_dict release];
}

bool iap_is_active(void) {
    return can_make_payments;
}

void iap_purchase(const char* _id) {    
    SKProduct *product = [products_dict objectForKey:[NSString stringWithUTF8String:_id]];
    SKPayment *payment = [SKPayment paymentWithProduct:product];
    [[SKPaymentQueue defaultQueue] addPayment:payment];
}

void _iap_updated_transaction(SKPaymentQueue* queue, SKPaymentTransaction* transaction) {
    NSString* key = transaction.payment.productIdentifier;
    const char* _id = [key UTF8String];
        
    switch (transaction.transactionState)
    {
        case SKPaymentTransactionStatePurchased:
            (*_iap_purchase_cb)(_id, true);
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            break;
        case SKPaymentTransactionStateRestored:
            (*_iap_purchase_cb)(_id, true);
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            break;
        case SKPaymentTransactionStateFailed:
            (*_iap_purchase_cb)(_id, false);
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            LOG_INFO([transaction.error.description UTF8String]);
            break;
        default:
            break;
    }
}

