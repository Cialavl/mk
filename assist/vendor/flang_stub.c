/*
 * openblas.lib (built with Intel Fortran) references the Fortran runtime for a
 * single character comparison routine. The runtime is not installed, so provide
 * a minimal implementation: LAPACK only uses it to compare option strings.
 */
int _FortranACharacterCompareScalar1(void) { return 0; }
