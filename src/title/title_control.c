/* Diagnostic source hypotheses, not canonical reconstruction or recovered names.
   External names encode independently observed TITLE.DLL destination RVAs. */
extern void title_04a40(void *data, int index);

void title_04a10(void *first, void *second)
{
    int index;
    for (index = 0; index < 18; ++index) {
        title_04a40(first, index);
        title_04a40(second, index);
    }
}

extern void title_0c610(int zero);
extern int title_06480(void);
extern void title_065a0(void);
extern void title_0c600(int zero);
extern int title_06530(void);
extern void title_064f0(void);
extern void title_0c620(int zero);

int title_06430(void)
{
    int result;
    title_0c610(0);
    result = title_06480();
    switch (result) {
    case 0:
        title_064f0();
        title_0c620(0);
        result = title_06480();
        break;
    case 2:
        break;
    case 3:
        title_065a0();
        title_0c600(0);
        title_06530();
        /* Fall through: case zero and default were separate source blocks.
           This hypothesis reproduces the oracle's redundant case-zero test. */
    default:
        title_064f0();
        title_0c620(0);
        result = title_06480();
        break;
    }
    return result;
}
