#include "demo_light_effect.h"
#include <FreeRTOS.h>
#include <bflb_gpio.h>
#include <semphr.h>
#include <stdio.h>
#include <task.h>
#include <timers.h>
static TimerHandle_t Light_TimerHdl     = NULL;
volatile static char Light_Timer_Status = 0;
static void demo_color_light_task(void);
static void Light_TimerHandler(TimerHandle_t p_timerhdl);
static void hard_set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat);
static uint32_t new_Rduty = 0;
static uint32_t new_Gduty = 0;
static uint32_t new_Bduty = 0;
static uint32_t new_Cduty = 0;
static uint32_t new_Wduty = 0;

static uint32_t Rduty = 0;
static uint32_t Gduty = 0;
static uint32_t Bduty = 0;
static uint32_t Cduty = 0;
static uint32_t Wduty = 0;
static uint16_t targettemp;
static uint8_t targetlevel;
// gamma = 2.00 steps = 3048 range = 0-1023
const float pwm_curve[] = {
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    6,    6,    6,    6,    6,    6,    6,
    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    8,    8,    8,    8,    8,    8,    8,    8,    8,    8,    8,    8,
    8,    8,    8,    8,    8,    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   11,   11,   11,   11,   11,   11,
    11,   11,   11,   11,   11,   11,   11,   11,   11,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
    12,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   16,   16,
    16,   16,   16,   16,   16,   16,   16,   16,   16,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
    18,   18,   18,   18,   18,   18,   18,   18,   18,   18,   18,   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
    19,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   21,   21,   21,   21,   21,   21,   21,   21,   21,
    21,   22,   22,   22,   22,   22,   22,   22,   22,   22,   22,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
    24,   24,   24,   24,   24,   24,   24,   24,   24,   24,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   26,
    26,   26,   26,   26,   26,   26,   26,   26,   27,   27,   27,   27,   27,   27,   27,   27,   27,   28,   28,   28,   28,
    28,   28,   28,   28,   28,   29,   29,   29,   29,   29,   29,   29,   29,   29,   30,   30,   30,   30,   30,   30,   30,
    30,   30,   31,   31,   31,   31,   31,   31,   31,   31,   32,   32,   32,   32,   32,   32,   32,   32,   32,   33,   33,
    33,   33,   33,   33,   33,   33,   34,   34,   34,   34,   34,   34,   34,   34,   35,   35,   35,   35,   35,   35,   35,
    35,   36,   36,   36,   36,   36,   36,   36,   36,   37,   37,   37,   37,   37,   37,   37,   37,   38,   38,   38,   38,
    38,   38,   38,   38,   39,   39,   39,   39,   39,   39,   39,   40,   40,   40,   40,   40,   40,   40,   40,   41,   41,
    41,   41,   41,   41,   41,   42,   42,   42,   42,   42,   42,   42,   42,   43,   43,   43,   43,   43,   43,   43,   44,
    44,   44,   44,   44,   44,   44,   45,   45,   45,   45,   45,   45,   45,   46,   46,   46,   46,   46,   46,   46,   47,
    47,   47,   47,   47,   47,   47,   48,   48,   48,   48,   48,   48,   48,   49,   49,   49,   49,   49,   49,   49,   50,
    50,   50,   50,   50,   50,   51,   51,   51,   51,   51,   51,   51,   52,   52,   52,   52,   52,   52,   52,   53,   53,
    53,   53,   53,   53,   54,   54,   54,   54,   54,   54,   54,   55,   55,   55,   55,   55,   55,   56,   56,   56,   56,
    56,   56,   56,   57,   57,   57,   57,   57,   57,   58,   58,   58,   58,   58,   58,   59,   59,   59,   59,   59,   59,
    60,   60,   60,   60,   60,   60,   61,   61,   61,   61,   61,   61,   61,   62,   62,   62,   62,   62,   62,   63,   63,
    63,   63,   63,   63,   64,   64,   64,   64,   64,   64,   65,   65,   65,   65,   65,   65,   66,   66,   66,   66,   66,
    67,   67,   67,   67,   67,   67,   68,   68,   68,   68,   68,   68,   69,   69,   69,   69,   69,   69,   70,   70,   70,
    70,   70,   71,   71,   71,   71,   71,   71,   72,   72,   72,   72,   72,   72,   73,   73,   73,   73,   73,   74,   74,
    74,   74,   74,   74,   75,   75,   75,   75,   75,   76,   76,   76,   76,   76,   76,   77,   77,   77,   77,   77,   78,
    78,   78,   78,   78,   78,   79,   79,   79,   79,   79,   80,   80,   80,   80,   80,   81,   81,   81,   81,   81,   81,
    82,   82,   82,   82,   82,   83,   83,   83,   83,   83,   84,   84,   84,   84,   84,   85,   85,   85,   85,   85,   86,
    86,   86,   86,   86,   86,   87,   87,   87,   87,   87,   88,   88,   88,   88,   88,   89,   89,   89,   89,   89,   90,
    90,   90,   90,   90,   91,   91,   91,   91,   91,   92,   92,   92,   92,   92,   93,   93,   93,   93,   93,   94,   94,
    94,   94,   94,   95,   95,   95,   95,   96,   96,   96,   96,   96,   97,   97,   97,   97,   97,   98,   98,   98,   98,
    98,   99,   99,   99,   99,   99,   100,  100,  100,  100,  100,  101,  101,  101,  101,  102,  102,  102,  102,  102,  103,
    103,  103,  103,  103,  104,  104,  104,  104,  105,  105,  105,  105,  105,  106,  106,  106,  106,  106,  107,  107,  107,
    107,  108,  108,  108,  108,  108,  109,  109,  109,  109,  110,  110,  110,  110,  110,  111,  111,  111,  111,  112,  112,
    112,  112,  112,  113,  113,  113,  113,  114,  114,  114,  114,  114,  115,  115,  115,  115,  116,  116,  116,  116,  116,
    117,  117,  117,  117,  118,  118,  118,  118,  118,  119,  119,  119,  119,  120,  120,  120,  120,  121,  121,  121,  121,
    121,  122,  122,  122,  122,  123,  123,  123,  123,  124,  124,  124,  124,  125,  125,  125,  125,  125,  126,  126,  126,
    126,  127,  127,  127,  127,  128,  128,  128,  128,  129,  129,  129,  129,  129,  130,  130,  130,  130,  131,  131,  131,
    131,  132,  132,  132,  132,  133,  133,  133,  133,  134,  134,  134,  134,  135,  135,  135,  135,  136,  136,  136,  136,
    136,  137,  137,  137,  137,  138,  138,  138,  138,  139,  139,  139,  139,  140,  140,  140,  140,  141,  141,  141,  141,
    142,  142,  142,  142,  143,  143,  143,  143,  144,  144,  144,  144,  145,  145,  145,  145,  146,  146,  146,  146,  147,
    147,  147,  148,  148,  148,  148,  149,  149,  149,  149,  150,  150,  150,  150,  151,  151,  151,  151,  152,  152,  152,
    152,  153,  153,  153,  153,  154,  154,  154,  154,  155,  155,  155,  156,  156,  156,  156,  157,  157,  157,  157,  158,
    158,  158,  158,  159,  159,  159,  159,  160,  160,  160,  161,  161,  161,  161,  162,  162,  162,  162,  163,  163,  163,
    163,  164,  164,  164,  165,  165,  165,  165,  166,  166,  166,  166,  167,  167,  167,  168,  168,  168,  168,  169,  169,
    169,  169,  170,  170,  170,  171,  171,  171,  171,  172,  172,  172,  172,  173,  173,  173,  174,  174,  174,  174,  175,
    175,  175,  175,  176,  176,  176,  177,  177,  177,  177,  178,  178,  178,  179,  179,  179,  179,  180,  180,  180,  181,
    181,  181,  181,  182,  182,  182,  183,  183,  183,  183,  184,  184,  184,  185,  185,  185,  185,  186,  186,  186,  187,
    187,  187,  187,  188,  188,  188,  189,  189,  189,  189,  190,  190,  190,  191,  191,  191,  191,  192,  192,  192,  193,
    193,  193,  193,  194,  194,  194,  195,  195,  195,  195,  196,  196,  196,  197,  197,  197,  198,  198,  198,  198,  199,
    199,  199,  200,  200,  200,  201,  201,  201,  201,  202,  202,  202,  203,  203,  203,  204,  204,  204,  204,  205,  205,
    205,  206,  206,  206,  207,  207,  207,  207,  208,  208,  208,  209,  209,  209,  210,  210,  210,  210,  211,  211,  211,
    212,  212,  212,  213,  213,  213,  214,  214,  214,  214,  215,  215,  215,  216,  216,  216,  217,  217,  217,  218,  218,
    218,  218,  219,  219,  219,  220,  220,  220,  221,  221,  221,  222,  222,  222,  222,  223,  223,  223,  224,  224,  224,
    225,  225,  225,  226,  226,  226,  227,  227,  227,  228,  228,  228,  228,  229,  229,  229,  230,  230,  230,  231,  231,
    231,  232,  232,  232,  233,  233,  233,  234,  234,  234,  235,  235,  235,  236,  236,  236,  236,  237,  237,  237,  238,
    238,  238,  239,  239,  239,  240,  240,  240,  241,  241,  241,  242,  242,  242,  243,  243,  243,  244,  244,  244,  245,
    245,  245,  246,  246,  246,  247,  247,  247,  248,  248,  248,  249,  249,  249,  250,  250,  250,  251,  251,  251,  252,
    252,  252,  253,  253,  253,  254,  254,  254,  255,  255,  255,  256,  256,  256,  257,  257,  257,  258,  258,  258,  259,
    259,  259,  260,  260,  260,  261,  261,  261,  262,  262,  262,  263,  263,  263,  264,  264,  264,  265,  265,  265,  266,
    266,  266,  267,  267,  267,  268,  268,  268,  269,  269,  270,  270,  270,  271,  271,  271,  272,  272,  272,  273,  273,
    273,  274,  274,  274,  275,  275,  275,  276,  276,  276,  277,  277,  278,  278,  278,  279,  279,  279,  280,  280,  280,
    281,  281,  281,  282,  282,  282,  283,  283,  283,  284,  284,  285,  285,  285,  286,  286,  286,  287,  287,  287,  288,
    288,  288,  289,  289,  290,  290,  290,  291,  291,  291,  292,  292,  292,  293,  293,  293,  294,  294,  295,  295,  295,
    296,  296,  296,  297,  297,  297,  298,  298,  299,  299,  299,  300,  300,  300,  301,  301,  301,  302,  302,  303,  303,
    303,  304,  304,  304,  305,  305,  305,  306,  306,  307,  307,  307,  308,  308,  308,  309,  309,  310,  310,  310,  311,
    311,  311,  312,  312,  312,  313,  313,  314,  314,  314,  315,  315,  315,  316,  316,  317,  317,  317,  318,  318,  318,
    319,  319,  320,  320,  320,  321,  321,  321,  322,  322,  323,  323,  323,  324,  324,  324,  325,  325,  326,  326,  326,
    327,  327,  327,  328,  328,  329,  329,  329,  330,  330,  331,  331,  331,  332,  332,  332,  333,  333,  334,  334,  334,
    335,  335,  336,  336,  336,  337,  337,  337,  338,  338,  339,  339,  339,  340,  340,  341,  341,  341,  342,  342,  342,
    343,  343,  344,  344,  344,  345,  345,  346,  346,  346,  347,  347,  348,  348,  348,  349,  349,  350,  350,  350,  351,
    351,  351,  352,  352,  353,  353,  353,  354,  354,  355,  355,  355,  356,  356,  357,  357,  357,  358,  358,  359,  359,
    359,  360,  360,  361,  361,  361,  362,  362,  363,  363,  363,  364,  364,  365,  365,  365,  366,  366,  367,  367,  367,
    368,  368,  369,  369,  369,  370,  370,  371,  371,  371,  372,  372,  373,  373,  373,  374,  374,  375,  375,  375,  376,
    376,  377,  377,  378,  378,  378,  379,  379,  380,  380,  380,  381,  381,  382,  382,  382,  383,  383,  384,  384,  384,
    385,  385,  386,  386,  387,  387,  387,  388,  388,  389,  389,  389,  390,  390,  391,  391,  392,  392,  392,  393,  393,
    394,  394,  394,  395,  395,  396,  396,  397,  397,  397,  398,  398,  399,  399,  399,  400,  400,  401,  401,  402,  402,
    402,  403,  403,  404,  404,  405,  405,  405,  406,  406,  407,  407,  407,  408,  408,  409,  409,  410,  410,  410,  411,
    411,  412,  412,  413,  413,  413,  414,  414,  415,  415,  416,  416,  416,  417,  417,  418,  418,  419,  419,  419,  420,
    420,  421,  421,  422,  422,  422,  423,  423,  424,  424,  425,  425,  425,  426,  426,  427,  427,  428,  428,  428,  429,
    429,  430,  430,  431,  431,  432,  432,  432,  433,  433,  434,  434,  435,  435,  435,  436,  436,  437,  437,  438,  438,
    439,  439,  439,  440,  440,  441,  441,  442,  442,  443,  443,  443,  444,  444,  445,  445,  446,  446,  446,  447,  447,
    448,  448,  449,  449,  450,  450,  450,  451,  451,  452,  452,  453,  453,  454,  454,  455,  455,  455,  456,  456,  457,
    457,  458,  458,  459,  459,  459,  460,  460,  461,  461,  462,  462,  463,  463,  464,  464,  464,  465,  465,  466,  466,
    467,  467,  468,  468,  468,  469,  469,  470,  470,  471,  471,  472,  472,  473,  473,  474,  474,  474,  475,  475,  476,
    476,  477,  477,  478,  478,  479,  479,  479,  480,  480,  481,  481,  482,  482,  483,  483,  484,  484,  485,  485,  485,
    486,  486,  487,  487,  488,  488,  489,  489,  490,  490,  491,  491,  491,  492,  492,  493,  493,  494,  494,  495,  495,
    496,  496,  497,  497,  498,  498,  499,  499,  499,  500,  500,  501,  501,  502,  502,  503,  503,  504,  504,  505,  505,
    506,  506,  507,  507,  507,  508,  508,  509,  509,  510,  510,  511,  511,  512,  512,  513,  513,  514,  514,  515,  515,
    516,  516,  516,  517,  517,  518,  518,  519,  519,  520,  520,  521,  521,  522,  522,  523,  523,  524,  524,  525,  525,
    526,  526,  527,  527,  528,  528,  528,  529,  529,  530,  530,  531,  531,  532,  532,  533,  533,  534,  534,  535,  535,
    536,  536,  537,  537,  538,  538,  539,  539,  540,  540,  541,  541,  542,  542,  543,  543,  544,  544,  545,  545,  545,
    546,  546,  547,  547,  548,  548,  549,  549,  550,  550,  551,  551,  552,  552,  553,  553,  554,  554,  555,  555,  556,
    556,  557,  557,  558,  558,  559,  559,  560,  560,  561,  561,  562,  562,  563,  563,  564,  564,  565,  565,  566,  566,
    567,  567,  568,  568,  569,  569,  570,  570,  571,  571,  572,  572,  573,  573,  574,  574,  575,  575,  576,  576,  577,
    577,  578,  578,  579,  579,  580,  580,  581,  581,  582,  582,  583,  583,  584,  584,  585,  585,  586,  586,  587,  587,
    588,  588,  589,  589,  590,  591,  591,  592,  592,  593,  593,  594,  594,  595,  595,  596,  596,  597,  597,  598,  598,
    599,  599,  600,  600,  601,  601,  602,  602,  603,  603,  604,  604,  605,  605,  606,  606,  607,  607,  608,  609,  609,
    610,  610,  611,  611,  612,  612,  613,  613,  614,  614,  615,  615,  616,  616,  617,  617,  618,  618,  619,  619,  620,
    620,  621,  622,  622,  623,  623,  624,  624,  625,  625,  626,  626,  627,  627,  628,  628,  629,  629,  630,  630,  631,
    632,  632,  633,  633,  634,  634,  635,  635,  636,  636,  637,  637,  638,  638,  639,  639,  640,  641,  641,  642,  642,
    643,  643,  644,  644,  645,  645,  646,  646,  647,  647,  648,  649,  649,  650,  650,  651,  651,  652,  652,  653,  653,
    654,  654,  655,  655,  656,  657,  657,  658,  658,  659,  659,  660,  660,  661,  661,  662,  662,  663,  664,  664,  665,
    665,  666,  666,  667,  667,  668,  668,  669,  670,  670,  671,  671,  672,  672,  673,  673,  674,  674,  675,  676,  676,
    677,  677,  678,  678,  679,  679,  680,  680,  681,  682,  682,  683,  683,  684,  684,  685,  685,  686,  686,  687,  688,
    688,  689,  689,  690,  690,  691,  691,  692,  693,  693,  694,  694,  695,  695,  696,  696,  697,  698,  698,  699,  699,
    700,  700,  701,  701,  702,  703,  703,  704,  704,  705,  705,  706,  706,  707,  708,  708,  709,  709,  710,  710,  711,
    711,  712,  713,  713,  714,  714,  715,  715,  716,  716,  717,  718,  718,  719,  719,  720,  720,  721,  722,  722,  723,
    723,  724,  724,  725,  726,  726,  727,  727,  728,  728,  729,  729,  730,  731,  731,  732,  732,  733,  733,  734,  735,
    735,  736,  736,  737,  737,  738,  739,  739,  740,  740,  741,  741,  742,  743,  743,  744,  744,  745,  745,  746,  747,
    747,  748,  748,  749,  749,  750,  751,  751,  752,  752,  753,  753,  754,  755,  755,  756,  756,  757,  758,  758,  759,
    759,  760,  760,  761,  762,  762,  763,  763,  764,  764,  765,  766,  766,  767,  767,  768,  769,  769,  770,  770,  771,
    771,  772,  773,  773,  774,  774,  775,  776,  776,  777,  777,  778,  778,  779,  780,  780,  781,  781,  782,  783,  783,
    784,  784,  785,  786,  786,  787,  787,  788,  788,  789,  790,  790,  791,  791,  792,  793,  793,  794,  794,  795,  796,
    796,  797,  797,  798,  799,  799,  800,  800,  801,  801,  802,  803,  803,  804,  804,  805,  806,  806,  807,  807,  808,
    809,  809,  810,  810,  811,  812,  812,  813,  813,  814,  815,  815,  816,  816,  817,  818,  818,  819,  819,  820,  821,
    821,  822,  822,  823,  824,  824,  825,  825,  826,  827,  827,  828,  828,  829,  830,  830,  831,  831,  832,  833,  833,
    834,  835,  835,  836,  836,  837,  838,  838,  839,  839,  840,  841,  841,  842,  842,  843,  844,  844,  845,  845,  846,
    847,  847,  848,  849,  849,  850,  850,  851,  852,  852,  853,  853,  854,  855,  855,  856,  856,  857,  858,  858,  859,
    860,  860,  861,  861,  862,  863,  863,  864,  864,  865,  866,  866,  867,  868,  868,  869,  869,  870,  871,  871,  872,
    873,  873,  874,  874,  875,  876,  876,  877,  877,  878,  879,  879,  880,  881,  881,  882,  882,  883,  884,  884,  885,
    886,  886,  887,  887,  888,  889,  889,  890,  891,  891,  892,  892,  893,  894,  894,  895,  896,  896,  897,  898,  898,
    899,  899,  900,  901,  901,  902,  903,  903,  904,  904,  905,  906,  906,  907,  908,  908,  909,  909,  910,  911,  911,
    912,  913,  913,  914,  915,  915,  916,  916,  917,  918,  918,  919,  920,  920,  921,  922,  922,  923,  923,  924,  925,
    925,  926,  927,  927,  928,  929,  929,  930,  931,  931,  932,  932,  933,  934,  934,  935,  936,  936,  937,  938,  938,
    939,  939,  940,  941,  941,  942,  943,  943,  944,  945,  945,  946,  947,  947,  948,  949,  949,  950,  950,  951,  952,
    952,  953,  954,  954,  955,  956,  956,  957,  958,  958,  959,  960,  960,  961,  962,  962,  963,  963,  964,  965,  965,
    966,  967,  967,  968,  969,  969,  970,  971,  971,  972,  973,  973,  974,  975,  975,  976,  977,  977,  978,  979,  979,
    980,  980,  981,  982,  982,  983,  984,  984,  985,  986,  986,  987,  988,  988,  989,  990,  990,  991,  992,  992,  993,
    994,  994,  995,  996,  996,  997,  998,  998,  999,  1000, 1000, 1001, 1002, 1002, 1003, 1004, 1004, 1005, 1006, 1006, 1007,
    1008, 1008, 1009, 1010, 1010, 1011, 1012, 1012, 1013, 1014, 1014, 1015, 1016, 1016, 1017, 1018, 1018, 1019, 1020, 1020, 1021,
    1022, 1022, 1023,
};

void demo_color_light_init(void)
{
    demo_color_light_task();
}

void demo_color_set_param(uint32_t p_Rduty, uint32_t p_Gduty, uint32_t p_Bduty, uint32_t p_Cduty, uint32_t p_Wduty)
{

    uint16_t threshold1, threshold2;
    threshold1 = 0;
#if 0
    bl_pwm_set_duty(LED_R_PIN_PORT, pwm_curve[p_Rduty]);
    bl_pwm_set_duty(LED_G_PIN_PORT, pwm_curve[p_Gduty]);
    bl_pwm_set_duty(LED_B_PIN_PORT, pwm_curve[p_Bduty]);

    if((pwm_curve[p_Cduty]+pwm_curve[p_Wduty])>100)
    {
        printf("%s >100",__func__);
    }
    else
    {
       
    }

    bl_pwm_set_duty_ex(LED_C_PIN_PORT, pwm_curve[p_Cduty], &threshold1, &threshold2);
    threshold1 = threshold2;
    bl_pwm_set_duty_ex(LED_W_PIN_PORT, pwm_curve[p_Wduty], &threshold1, &threshold2);
#endif
}

void set_level(uint8_t currLevel)
{
    printf("%s\r\n", __func__);
}

void set_color_red(uint8_t currLevel)
{
    hard_set_color(currLevel, 0, 254);
}

void set_color_green(uint8_t currLevel)
{
    hard_set_color(currLevel, 84, 254);
}

void set_color_yellow(uint8_t currLevel)
{
    hard_set_color(currLevel, 42, 254);
}

static void hard_set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat)
{
#if MAX_PWM_CHANNEL
    uint16_t hue = (uint16_t) currHue * 360 / 254;
    uint8_t sat  = (uint16_t) currSat * 100 / 254;

    if (currLevel <= 5 && currLevel >= 1)
    {
        currLevel = 5; // avoid demo off
    }

    if (sat > 100)
    {
        sat = 100;
    }

    uint16_t i       = hue / 60;
    uint16_t rgb_max = currLevel;
    uint16_t rgb_min = rgb_max * (100 - sat) / 100;
    uint16_t diff    = hue % 60;
    uint16_t rgb_adj = (rgb_max - rgb_min) * diff / 60;
    uint32_t red, green, blue;

    switch (i)
    {
    case 0:
        red   = rgb_max;
        green = rgb_min + rgb_adj;
        blue  = rgb_min;
        break;
    case 1:
        red   = rgb_max - rgb_adj;
        green = rgb_max;
        blue  = rgb_min;
        break;
    case 2:
        red   = rgb_min;
        green = rgb_max;
        blue  = rgb_min + rgb_adj;
        break;
    case 3:
        red   = rgb_min;
        green = rgb_max - rgb_adj;
        blue  = rgb_max;
        break;
    case 4:
        red   = rgb_min + rgb_adj;
        green = rgb_min;
        blue  = rgb_max;
        break;
    default:
        red   = rgb_max;
        green = rgb_min;
        blue  = rgb_max - rgb_adj;
        break;
    }

    Rduty = (red * 12);
    Gduty = (green * 12);
    Bduty = (blue * 12);
    printf("Rduty=%lx,Gduty=%lx,Bduty=%lx\r\n", Rduty, Gduty, Bduty);
    demo_color_set_param(Rduty, Gduty, Bduty, 0, 0);
#else
    set_level(currLevel);
#endif
}
int get_curve_value(uint32_t temp)
{
    int value   = 0;
    float temp1 = (float) (temp) / 254.000;
    temp1       = temp1 * 100;

    printf("temp1 %f\r\n", temp1);

    while (1)
    {
        if ((pwm_curve[value] <= temp1) && (pwm_curve[value + 1] >= temp1))
        {
            break;
        }
        value++;
    }
    return value;
}
void set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat)
{
    printf("%s\r\n", __func__);
    uint16_t hue = (uint16_t) currHue * 360 / 254;
    uint8_t sat  = (uint16_t) currSat * 100 / 254;

    if (sat > 100)
    {
        sat = 100;
    }

    uint16_t i       = hue / 60;
    uint16_t rgb_max = currLevel;
    uint16_t rgb_min = rgb_max * (100 - sat) / 100;
    uint16_t diff    = hue % 60;
    uint16_t rgb_adj = (rgb_max - rgb_min) * diff / 60;
    uint32_t red, green, blue;

    switch (i)
    {
    case 0:
        red   = rgb_max;
        green = rgb_min + rgb_adj;
        blue  = rgb_min;
        break;
    case 1:
        red   = rgb_max - rgb_adj;
        green = rgb_max;
        blue  = rgb_min;
        break;
    case 2:
        red   = rgb_min;
        green = rgb_max;
        blue  = rgb_min + rgb_adj;
        break;
    case 3:
        red   = rgb_min;
        green = rgb_max - rgb_adj;
        blue  = rgb_max;
        break;
    case 4:
        red   = rgb_min + rgb_adj;
        green = rgb_min;
        blue  = rgb_max;
        break;
    default:
        red   = rgb_max;
        green = rgb_min;
        blue  = rgb_max - rgb_adj;
        break;
    }

    new_Rduty = (red * 12);
    new_Gduty = (green * 12);
    new_Bduty = (blue * 12);
    printf("now_Rduty update=%lx,now_Gduty update =%lx,now_Bduty update     =%lx\r\n", new_Rduty, new_Gduty, new_Bduty);
    if (Light_TimerHdl != NULL)
    {
        if (xTimerIsTimerActive(Light_TimerHdl) != pdFALSE)
        {
            if (Light_TimerHdl)
                xTimerStop(Light_TimerHdl, 0);
        }
        if (xTimerChangePeriod(Light_TimerHdl, pdMS_TO_TICKS(1), 0) == pdPASS)
        {

            Light_Timer_Status = 1;
            if (Light_TimerHdl)
                xTimerStart(Light_TimerHdl, 0);
        }
    }
}
void set_temperature(uint8_t currLevel, uint16_t temperature)
{

    if ((targettemp != temperature) || (targetlevel != currLevel))
    {
        targettemp  = temperature;
        targetlevel = currLevel;
        printf("%s\r\n", __func__);
        uint32_t hw_temp_delta = LAM_MAX_MIREDS_DEFAULT - LAM_MIN_MIREDS_DEFAULT;
        uint32_t soft_temp_delta;

        if (temperature > LAM_MAX_MIREDS_DEFAULT)
        {
            temperature = LAM_MAX_MIREDS_DEFAULT;
        }
        else if (temperature < LAM_MIN_MIREDS_DEFAULT)
        {
            temperature = LAM_MIN_MIREDS_DEFAULT;
        }

        soft_temp_delta = temperature - LAM_MIN_MIREDS_DEFAULT;
        soft_temp_delta *= 100;

        uint32_t warm = (254 * (soft_temp_delta / hw_temp_delta)) / 100;
        uint32_t clod = 254 - warm;

        warm      = warm * currLevel / 254;
        clod      = clod * currLevel / 254;
        new_Wduty = get_curve_value(warm);
        new_Cduty = get_curve_value(clod);
        printf("now_Cduty update=%lx,now_Wduty update =%lx\r\n", new_Cduty, new_Wduty);
        if (Light_TimerHdl != NULL)
        {
            if (xTimerIsTimerActive(Light_TimerHdl) != pdFALSE)
            {
                if (Light_TimerHdl)
                    xTimerStop(Light_TimerHdl, 0);
            }
            if (xTimerChangePeriod(Light_TimerHdl, pdMS_TO_TICKS(1), 0) == pdPASS)
            {

                Light_Timer_Status = 2;
                if (Light_TimerHdl)
                    xTimerStart(Light_TimerHdl, 0);
            }
        }
    }
}
void set_warm_temperature(void)
{
    uint16_t threshold1, threshold2;
    targetlevel            = 254;
    targettemp             = 454;
    uint32_t hw_temp_delta = LAM_MAX_MIREDS_DEFAULT - LAM_MIN_MIREDS_DEFAULT;
    uint32_t soft_temp_delta;

    if (targettemp > LAM_MAX_MIREDS_DEFAULT)
    {
        targettemp = LAM_MAX_MIREDS_DEFAULT;
    }
    else if (targettemp < LAM_MIN_MIREDS_DEFAULT)
    {
        targettemp = LAM_MIN_MIREDS_DEFAULT;
    }

    soft_temp_delta = targettemp - LAM_MIN_MIREDS_DEFAULT;
    soft_temp_delta *= 100;

    uint32_t warm = (254 * (soft_temp_delta / hw_temp_delta)) / 100;
    uint32_t clod = 254 - warm;
    Cduty         = 0;
    Wduty         = 0xbe7;
    Rduty         = 0;
    Gduty         = 0;
    Bduty         = 0;
    printf("now_Cduty update=%lx,now_Wduty update =%lx\r\n", Cduty, Wduty);
#if 0
    bl_pwm_set_duty(LED_R_PIN_PORT, pwm_curve[Rduty]);
    bl_pwm_set_duty(LED_G_PIN_PORT, pwm_curve[Gduty]);
    bl_pwm_set_duty(LED_B_PIN_PORT, pwm_curve[Bduty]);
    bl_pwm_set_duty_ex(LED_C_PIN_PORT, pwm_curve[Cduty], &threshold1, &threshold2);
    threshold1 = threshold2;
    bl_pwm_set_duty_ex(LED_W_PIN_PORT, pwm_curve[Wduty], &threshold1, &threshold2);
#endif
}

void set_cold_temperature(void)
{
    uint16_t threshold1, threshold2;
    targetlevel            = 254;
    targettemp             = 370;
    uint32_t hw_temp_delta = LAM_MAX_MIREDS_DEFAULT - LAM_MIN_MIREDS_DEFAULT;
    uint32_t soft_temp_delta;

    if (targettemp > LAM_MAX_MIREDS_DEFAULT)
    {
        targettemp = LAM_MAX_MIREDS_DEFAULT;
    }
    else if (targettemp < LAM_MIN_MIREDS_DEFAULT)
    {
        targettemp = LAM_MIN_MIREDS_DEFAULT;
    }

    soft_temp_delta = targettemp - LAM_MIN_MIREDS_DEFAULT;
    soft_temp_delta *= 100;

    uint32_t warm = (254 * (soft_temp_delta / hw_temp_delta)) / 100;
    uint32_t clod = 254 - warm;
    Cduty         = 0x8a5;
    Wduty         = 0xB0B;

    threshold1 = 0;
    Rduty      = 0;
    Gduty      = 0;
    Bduty      = 0;
    printf("now_Cduty update=%lx,now_Wduty update =%lx\r\n", Cduty, Wduty);
#if 0
    bl_pwm_set_duty_ex(LED_C_PIN_PORT, pwm_curve[Cduty], &threshold1, &threshold2);
    threshold1 = threshold2;
    bl_pwm_set_duty_ex(LED_W_PIN_PORT, pwm_curve[Wduty], &threshold1, &threshold2);
#endif
}

static void Light_TimerHandler(TimerHandle_t p_timerhdl)
{

    if (Light_Timer_Status == 1)
    {
        if ((new_Rduty != Rduty) || (new_Gduty != Gduty) || (new_Bduty != Bduty))
        {
            if (new_Rduty > Rduty)
            {
                Rduty++;
            }
            else if (new_Rduty < Rduty)
            {
                Rduty--;
            }

            if (new_Gduty > Gduty)
            {
                Gduty++;
            }
            else if (new_Gduty < Gduty)
            {
                Gduty--;
            }

            if (new_Bduty > Bduty)
            {
                Bduty++;
            }
            else if (new_Bduty < Bduty)
            {
                Bduty--;
            }
            demo_color_set_param(Rduty, Gduty, Bduty, 0, 0);
        }
        else
        {
            if (Light_TimerHdl)
            {
                xTimerStop(Light_TimerHdl, 0);
            }
            Light_Timer_Status = 0;
        }
    }
    else if (Light_Timer_Status == 2)
    {
        if ((new_Cduty != Cduty) || (new_Wduty != Wduty))
        {
            if ((Cduty == 0) && (Wduty == 0))
            {
                if (new_Wduty >= 0xbe7)
                {
                    Wduty = 0xbe7;
                }
                else
                {
                    Wduty = new_Wduty;
                }

                if (new_Cduty >= 0xbe7)
                {
                    Cduty = 0xbe7;
                }
                else
                {
                    Cduty = new_Cduty;
                }
            }
            if (new_Cduty > Cduty)
            {
                Cduty++;
            }
            else if (new_Cduty < Cduty)
            {
                Cduty--;
            }

            if (new_Wduty > Wduty)
            {
                Wduty++;
            }
            else if (new_Wduty < Wduty)
            {
                Wduty--;
            }
            if (targetlevel == 0)
            {
                if ((Cduty <= 0xbe7) && (Wduty <= 0xbe7))
                {
                    Cduty = 0;
                    Wduty = 0;
                }
            }

            demo_color_set_param(0, 0, 0, Cduty, Wduty);
        }
        else
        {
            if (Light_TimerHdl)
            {
                xTimerStop(Light_TimerHdl, 0);
            }
            Light_Timer_Status = 0;
        }
    }
}
static void demo_color_light_task(void)
{

    Light_TimerHdl = xTimerCreate("Light_TimerHandler", pdMS_TO_TICKS(2), pdTRUE, NULL, Light_TimerHandler);
}