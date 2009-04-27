#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_formosa.h"

static function_entry formosa_functions[] = {
    PHP_FE(formosa_world, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry formosa_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_FORMOSA_EXTNAME,
    formosa_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_FORMOSA_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_FORMOSA
ZEND_GET_MODULE(formosa)
#endif

PHP_FUNCTION(formosa_world)
{
    RETURN_STRING("Formosa World", 1);
}
