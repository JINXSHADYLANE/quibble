#include "iap.h"

#include "datastruct.h"
#include "darray.h"

#import <StoreKit/StoreKit.h>
#import "DGreedAppDelegate.h"

static DGreedAppDelegate* app_delegate = nil;
static bool can_make_payments = false;
static NSMutableDictionary* products_dict = NULL;
static NSMutableDictionary* prices_dict = NULL;
static Dict callbacks_dict;

void _set_iap_app_delegate(DGreedAppDelegate* _app_delegate) {
    app_delegate = _app_delegate;
}

void iap_init(void) {
    if ([SKPaymentQueue canMakePayments]) {
        can_make_payments = true;
        
        products_dict = [[NSMutableDictionary alloc] init];
        prices_dict = [[NSMutableDictionary alloc] init];
        dict_init(&callbacks_dict);
        
        [[SKPaymentQueue defaultQueue] addTransactionObserver:app_delegate];
        
        SKProductsRequest *request= [[SKProductsRequest alloc] initWithProductIdentifiers:
                                     [NSSet setWithObject: @"unlock"]];
        request.delegate = app_delegate;
        [request start];
    }
}

void _iap_received_products_response(SKProductsResponse* response) {
    NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
    
    for(SKProduct* product in response.products) {
        [products_dict setValue:product forKey:product.productIdentifier];
        [numberFormatter setLocale:product.priceLocale];
        [prices_dict setValue:[numberFormatter stringFromNumber:product.price] forKey:product.productIdentifier];
    }
}

void iap_close(void) {
    if(products_dict)
        [products_dict release];
    if(prices_dict) {
        [prices_dict release];
        dict_free(&callbacks_dict);
    }
}

bool iap_is_active(void) {
    return can_make_payments;
}

void iap_get_products(ProductCallback cb) {
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

void iap_purchase(const char* _id, PurchaseCallback cb) {
    dict_set(&callbacks_dict, _id, cb);
    
    SKProduct *product = [products_dict objectForKey:[NSString stringWithUTF8String:_id]];
    SKPayment *payment = [SKPayment paymentWithProduct:product];
    [[SKPaymentQueue defaultQueue] addPayment:payment];
    
    // Temporary, for testing!
    (*cb)(_id, true);
}

void _iap_updated_transaction(SKPaymentQueue* queue, SKPaymentTransaction* transaction) {
    NSString* key = transaction.payment.productIdentifier;
    const char* _id = [key UTF8String];
    PurchaseCallback cb = dict_get(&callbacks_dict, _id);
    if(cb == NULL) {
        LOG_WARNING("No callback for payment id %s", _id);
        return;
    }
    
    switch (transaction.transactionState)
    {
        case SKPaymentTransactionStatePurchased:
        case SKPaymentTransactionStateRestored:
            (*cb)(_id, true);
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            break;
        case SKPaymentTransactionStateFailed:
            if(transaction.error.code != SKErrorPaymentCancelled)
                (*cb)(_id, false);
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            break;
        default:
            break;
    }
}

