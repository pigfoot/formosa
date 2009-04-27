#ifndef PHP_FORMOSA_H
#define PHP_FORMOSA_H 1

#define PHP_FORMOSA_VERSION "1.0"
#define PHP_FORMOSA_EXTNAME "formosa"

PHP_FUNCTION(formosa_world);

extern zend_module_entry formosa_module_entry;
#define phpext_formosa_ptr &formosa_module_entry

#endif
