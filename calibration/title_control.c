/* Diagnostic source hypotheses, not canonical reconstruction or recovered names.
   External names encode independently observed TITLE.DLL destination RVAs. */
extern void title_4a40(void *data, int index);

void title_4a10(void *first, void *second)
{
    int index;
    for (index = 0; index < 18; ++index) {
        title_4a40(first, index);
        title_4a40(second, index);
    }
}

extern void title_c610(int zero);
extern int title_6480(void);
extern void title_65a0(void);
extern void title_c600(int zero);
extern int title_6530(void);
extern void title_64f0(void);
extern void title_c620(int zero);

int title_6430(void)
{
    int result;
    title_c610(0);
    result = title_6480();
    switch (result) {
    case 0:
        title_64f0();
        title_c620(0);
        result = title_6480();
        break;
    case 2:
        break;
    case 3:
        title_65a0();
        title_c600(0);
        title_6530();
        /* Fall through: case zero and default were separate source blocks.
           This hypothesis reproduces the oracle's redundant case-zero test. */
    default:
        title_64f0();
        title_c620(0);
        result = title_6480();
        break;
    }
    return result;
}
