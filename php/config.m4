PHP_ARG_ENABLE(formosa, whether to enable Hello World support,
[ --enable-formosa   Enable Hello World support])

if test "$PHP_FORMOSA" = "yes"; then
  AC_DEFINE(HAVE_FORMOSA, 1, [Whether you have Hello World])
  PHP_NEW_EXTENSION(formosa, formosa.c, $ext_shared)
fi
