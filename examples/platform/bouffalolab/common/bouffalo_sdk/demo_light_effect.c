#include "demo_light_effect.h"
#include "SM2235EGH.h"
#include <FreeRTOS.h>
#include <bflb_gpio.h>
#include <math.h>
#include <semphr.h>
#include <stdio.h>
#include <task.h>
#include <timers.h>
static TimerHandle_t Light_TimerHdl = NULL;
static char Light_Timer_Status      = 0;
static void demo_color_light_task(void);
static void Light_TimerHandler(TimerHandle_t p_timerhdl);
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
static uint8_t lastlevel=0;
// gamma = 2.00 steps = 3048 range = 0-1023
const float pwm_curve[] = {
    0,    10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,  10,  10,  10,  10,  10,  10,  10,   10,   10,
    10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   11,  11,  11,  11,  11,  11,  11,  11,   11,   11,
    11,   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,  11,  11,  11,  11,  11,  11,  11,   11,   11,
    11,   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,  11,  11,  11,  11,  11,  11,  11,   11,   11,
    11,   11,   11,   11,   11,   12,   12,   12,   12,   12,   12,   12,   12,  12,  12,  12,  12,  12,  12,  12,   12,   12,
    12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,  12,  12,  12,  12,  12,  12,  12,   12,   12,
    12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,  12,  12,  12,  13,  13,  13,  13,   13,   13,
    13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,  13,  13,  13,  13,  13,  13,  13,   13,   13,
    13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,  13,  13,  13,  13,  13,  13,  13,   13,   13,
    13,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,  14,  14,  14,  14,  14,  14,  14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,  14,  14,  14,  14,  14,  14,  14,   14,   14,
    14,   14,   14,   14,   15,   15,   15,   15,   15,   15,   15,   15,   15,  15,  15,  15,  15,  15,  15,  15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,  15,  15,  15,  15,  15,  15,  15,   15,   15,
    15,   15,   15,   15,   16,   16,   16,   16,   16,   16,   16,   16,   16,  16,  16,  16,  16,  16,  16,  16,   16,   16,
    16,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16,  16,  16,  16,  16,  16,  16,  16,   16,   16,
    16,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,  17,  17,  17,  17,  17,  17,  17,   17,   17,
    17,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,  17,  17,  17,  17,  17,  18,  18,   18,   18,
    18,   18,   18,   18,   18,   18,   18,   18,   18,   18,   18,   18,   18,  18,  18,  18,  18,  18,  18,  18,   18,   18,
    18,   18,   18,   18,   18,   18,   18,   18,   18,   18,   18,   19,   19,  19,  19,  19,  19,  19,  19,  19,   19,   19,
    19,   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,  19,  19,  19,  19,  19,  19,  19,   19,   19,
    19,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,  20,  20,  20,  20,  20,  20,  20,   20,   20,
    20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   21,  21,  21,  21,  21,  21,  21,  21,   21,   21,
    21,   21,   21,   21,   21,   21,   21,   21,   21,   21,   21,   21,   21,  21,  21,  21,  21,  21,  21,  21,   21,   22,
    22,   22,   22,   22,   22,   22,   22,   22,   22,   22,   22,   22,   22,  22,  22,  22,  22,  22,  22,  22,   22,   22,
    22,   22,   22,   22,   22,   22,   22,   23,   23,   23,   23,   23,   23,  23,  23,  23,  23,  23,  23,  23,   23,   23,
    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,  23,  24,  24,  24,  24,  24,  24,   24,   24,
    24,   24,   24,   24,   24,   24,   24,   24,   24,   24,   24,   24,   24,  24,  24,  24,  24,  24,  24,  25,   25,   25,
    25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,  25,  25,  25,  25,  25,  25,  25,   25,   25,
    25,   25,   26,   26,   26,   26,   26,   26,   26,   26,   26,   26,   26,  26,  26,  26,  26,  26,  26,  26,   26,   26,
    26,   26,   26,   26,   26,   27,   27,   27,   27,   27,   27,   27,   27,  27,  27,  27,  27,  27,  27,  27,   27,   27,
    27,   27,   27,   27,   27,   27,   27,   27,   28,   28,   28,   28,   28,  28,  28,  28,  28,  28,  28,  28,   28,   28,
    28,   28,   28,   28,   28,   28,   28,   28,   28,   29,   29,   29,   29,  29,  29,  29,  29,  29,  29,  29,   29,   29,
    29,   29,   29,   29,   29,   29,   29,   29,   29,   29,   30,   30,   30,  30,  30,  30,  30,  30,  30,  30,   30,   30,
    30,   30,   30,   30,   30,   30,   30,   30,   30,   30,   31,   31,   31,  31,  31,  31,  31,  31,  31,  31,   31,   31,
    31,   31,   31,   31,   31,   31,   31,   31,   31,   32,   32,   32,   32,  32,  32,  32,  32,  32,  32,  32,   32,   32,
    32,   32,   32,   32,   32,   32,   32,   32,   33,   33,   33,   33,   33,  33,  33,  33,  33,  33,  33,  33,   33,   33,
    33,   33,   33,   33,   33,   34,   34,   34,   34,   34,   34,   34,   34,  34,  34,  34,  34,  34,  34,  34,   34,   34,
    34,   34,   34,   35,   35,   35,   35,   35,   35,   35,   35,   35,   35,  35,  35,  35,  35,  35,  35,  35,   35,   35,
    36,   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,  36,  36,  36,  36,  36,  37,  37,   37,   37,
    37,   37,   37,   37,   37,   37,   37,   37,   37,   37,   37,   37,   37,  37,  38,  38,  38,  38,  38,  38,   38,   38,
    38,   38,   38,   38,   38,   38,   38,   38,   38,   39,   39,   39,   39,  39,  39,  39,  39,  39,  39,  39,   39,   39,
    39,   39,   39,   39,   40,   40,   40,   40,   40,   40,   40,   40,   40,  40,  40,  40,  40,  40,  40,  40,   41,   41,
    41,   41,   41,   41,   41,   41,   41,   41,   41,   41,   41,   41,   41,  41,  42,  42,  42,  42,  42,  42,   42,   42,
    42,   42,   42,   42,   42,   42,   42,   42,   43,   43,   43,   43,   43,  43,  43,  43,  43,  43,  43,  43,   43,   43,
    43,   44,   44,   44,   44,   44,   44,   44,   44,   44,   44,   44,   44,  44,  44,  44,  45,  45,  45,  45,   45,   45,
    45,   45,   45,   45,   45,   45,   45,   45,   45,   46,   46,   46,   46,  46,  46,  46,  46,  46,  46,  46,   46,   46,
    46,   47,   47,   47,   47,   47,   47,   47,   47,   47,   47,   47,   47,  47,  47,  48,  48,  48,  48,  48,   48,   48,
    48,   48,   48,   48,   48,   48,   48,   49,   49,   49,   49,   49,   49,  49,  49,  49,  49,  49,  49,  49,   49,   50,
    50,   50,   50,   50,   50,   50,   50,   50,   50,   50,   50,   50,   51,  51,  51,  51,  51,  51,  51,  51,   51,   51,
    51,   51,   51,   52,   52,   52,   52,   52,   52,   52,   52,   52,   52,  52,  52,  53,  53,  53,  53,  53,   53,   53,
    53,   53,   53,   53,   53,   53,   54,   54,   54,   54,   54,   54,   54,  54,  54,  54,  54,  54,  55,  55,   55,   55,
    55,   55,   55,   55,   55,   55,   55,   55,   56,   56,   56,   56,   56,  56,  56,  56,  56,  56,  56,  56,   57,   57,
    57,   57,   57,   57,   57,   57,   57,   57,   57,   58,   58,   58,   58,  58,  58,  58,  58,  58,  58,  58,   58,   59,
    59,   59,   59,   59,   59,   59,   59,   59,   59,   59,   60,   60,   60,  60,  60,  60,  60,  60,  60,  60,   60,   61,
    61,   61,   61,   61,   61,   61,   61,   61,   61,   62,   62,   62,   62,  62,  62,  62,  62,  62,  62,  62,   63,   63,
    63,   63,   63,   63,   63,   63,   63,   63,   63,   64,   64,   64,   64,  64,  64,  64,  64,  64,  64,  65,   65,   65,
    65,   65,   65,   65,   65,   65,   65,   66,   66,   66,   66,   66,   66,  66,  66,  66,  66,  67,  67,  67,   67,   67,
    67,   67,   67,   67,   67,   68,   68,   68,   68,   68,   68,   68,   68,  68,  69,  69,  69,  69,  69,  69,   69,   69,
    69,   69,   70,   70,   70,   70,   70,   70,   70,   70,   70,   71,   71,  71,  71,  71,  71,  71,  71,  71,   71,   72,
    72,   72,   72,   72,   72,   72,   72,   72,   73,   73,   73,   73,   73,  73,  73,  73,  73,  74,  74,  74,   74,   74,
    74,   74,   74,   74,   75,   75,   75,   75,   75,   75,   75,   75,   76,  76,  76,  76,  76,  76,  76,  76,   76,   77,
    77,   77,   77,   77,   77,   77,   77,   77,   78,   78,   78,   78,   78,  78,  78,  78,  79,  79,  79,  79,   79,   79,
    79,   79,   80,   80,   80,   80,   80,   80,   80,   80,   80,   81,   81,  81,  81,  81,  81,  81,  81,  82,   82,   82,
    82,   82,   82,   82,   82,   83,   83,   83,   83,   83,   83,   83,   83,  84,  84,  84,  84,  84,  84,  84,   84,   85,
    85,   85,   85,   85,   85,   85,   86,   86,   86,   86,   86,   86,   86,  86,  87,  87,  87,  87,  87,  87,   87,   87,
    88,   88,   88,   88,   88,   88,   88,   89,   89,   89,   89,   89,   89,  89,  90,  90,  90,  90,  90,  90,   90,   90,
    91,   91,   91,   91,   91,   91,   91,   92,   92,   92,   92,   92,   92,  92,  93,  93,  93,  93,  93,  93,   93,   94,
    94,   94,   94,   94,   94,   94,   95,   95,   95,   95,   95,   95,   95,  96,  96,  96,  96,  96,  96,  96,   97,   97,
    97,   97,   97,   97,   97,   98,   98,   98,   98,   98,   98,   98,   99,  99,  99,  99,  99,  99,  100, 100,  100,  100,
    100,  100,  100,  101,  101,  101,  101,  101,  101,  102,  102,  102,  102, 102, 102, 102, 103, 103, 103, 103,  103,  103,
    104,  104,  104,  104,  104,  104,  105,  105,  105,  105,  105,  105,  105, 106, 106, 106, 106, 106, 106, 107,  107,  107,
    107,  107,  107,  108,  108,  108,  108,  108,  108,  109,  109,  109,  109, 109, 109, 110, 110, 110, 110, 110,  110,  111,
    111,  111,  111,  111,  111,  112,  112,  112,  112,  112,  112,  113,  113, 113, 113, 113, 113, 114, 114, 114,  114,  114,
    114,  115,  115,  115,  115,  115,  116,  116,  116,  116,  116,  116,  117, 117, 117, 117, 117, 117, 118, 118,  118,  118,
    118,  119,  119,  119,  119,  119,  119,  120,  120,  120,  120,  120,  121, 121, 121, 121, 121, 121, 122, 122,  122,  122,
    122,  123,  123,  123,  123,  123,  124,  124,  124,  124,  124,  124,  125, 125, 125, 125, 125, 126, 126, 126,  126,  126,
    127,  127,  127,  127,  127,  128,  128,  128,  128,  128,  128,  129,  129, 129, 129, 129, 130, 130, 130, 130,  130,  131,
    131,  131,  131,  131,  132,  132,  132,  132,  132,  133,  133,  133,  133, 133, 134, 134, 134, 134, 134, 135,  135,  135,
    135,  135,  136,  136,  136,  136,  137,  137,  137,  137,  137,  138,  138, 138, 138, 138, 139, 139, 139, 139,  139,  140,
    140,  140,  140,  141,  141,  141,  141,  141,  142,  142,  142,  142,  142, 143, 143, 143, 143, 144, 144, 144,  144,  144,
    145,  145,  145,  145,  146,  146,  146,  146,  146,  147,  147,  147,  147, 148, 148, 148, 148, 148, 149, 149,  149,  149,
    150,  150,  150,  150,  150,  151,  151,  151,  151,  152,  152,  152,  152, 153, 153, 153, 153, 153, 154, 154,  154,  154,
    155,  155,  155,  155,  156,  156,  156,  156,  157,  157,  157,  157,  157, 158, 158, 158, 158, 159, 159, 159,  159,  160,
    160,  160,  160,  161,  161,  161,  161,  162,  162,  162,  162,  163,  163, 163, 163, 164, 164, 164, 164, 165,  165,  165,
    165,  166,  166,  166,  166,  167,  167,  167,  167,  168,  168,  168,  168, 169, 169, 169, 169, 170, 170, 170,  170,  171,
    171,  171,  171,  172,  172,  172,  173,  173,  173,  173,  174,  174,  174, 174, 175, 175, 175, 175, 176, 176,  176,  176,
    177,  177,  177,  178,  178,  178,  178,  179,  179,  179,  179,  180,  180, 180, 181, 181, 181, 181, 182, 182,  182,  182,
    183,  183,  183,  184,  184,  184,  184,  185,  185,  185,  186,  186,  186, 186, 187, 187, 187, 188, 188, 188,  188,  189,
    189,  189,  190,  190,  190,  190,  191,  191,  191,  192,  192,  192,  192, 193, 193, 193, 194, 194, 194, 194,  195,  195,
    195,  196,  196,  196,  197,  197,  197,  197,  198,  198,  198,  199,  199, 199, 200, 200, 200, 200, 201, 201,  201,  202,
    202,  202,  203,  203,  203,  204,  204,  204,  204,  205,  205,  205,  206, 206, 206, 207, 207, 207, 208, 208,  208,  209,
    209,  209,  210,  210,  210,  210,  211,  211,  211,  212,  212,  212,  213, 213, 213, 214, 214, 214, 215, 215,  215,  216,
    216,  216,  217,  217,  217,  218,  218,  218,  219,  219,  219,  220,  220, 220, 221, 221, 221, 222, 222, 222,  223,  223,
    223,  224,  224,  224,  225,  225,  225,  226,  226,  226,  227,  227,  227, 228, 228, 228, 229, 229, 230, 230,  230,  231,
    231,  231,  232,  232,  232,  233,  233,  233,  234,  234,  234,  235,  235, 236, 236, 236, 237, 237, 237, 238,  238,  238,
    239,  239,  239,  240,  240,  241,  241,  241,  242,  242,  242,  243,  243, 244, 244, 244, 245, 245, 245, 246,  246,  246,
    247,  247,  248,  248,  248,  249,  249,  250,  250,  250,  251,  251,  251, 252, 252, 253, 253, 253, 254, 254,  254,  255,
    255,  256,  256,  256,  257,  257,  258,  258,  258,  259,  259,  260,  260, 260, 261, 261, 262, 262, 262, 263,  263,  264,
    264,  264,  265,  265,  266,  266,  266,  267,  267,  268,  268,  268,  269, 269, 270, 270, 270, 271, 271, 272,  272,  272,
    273,  273,  274,  274,  275,  275,  275,  276,  276,  277,  277,  277,  278, 278, 279, 279, 280, 280, 280, 281,  281,  282,
    282,  283,  283,  283,  284,  284,  285,  285,  286,  286,  286,  287,  287, 288, 288, 289, 289, 290, 290, 290,  291,  291,
    292,  292,  293,  293,  294,  294,  294,  295,  295,  296,  296,  297,  297, 298, 298, 298, 299, 299, 300, 300,  301,  301,
    302,  302,  303,  303,  304,  304,  304,  305,  305,  306,  306,  307,  307, 308, 308, 309, 309, 310, 310, 311,  311,  311,
    312,  312,  313,  313,  314,  314,  315,  315,  316,  316,  317,  317,  318, 318, 319, 319, 320, 320, 321, 321,  322,  322,
    323,  323,  324,  324,  324,  325,  325,  326,  326,  327,  327,  328,  328, 329, 329, 330, 330, 331, 331, 332,  332,  333,
    333,  334,  334,  335,  336,  336,  337,  337,  338,  338,  339,  339,  340, 340, 341, 341, 342, 342, 343, 343,  344,  344,
    345,  345,  346,  346,  347,  347,  348,  348,  349,  350,  350,  351,  351, 352, 352, 353, 353, 354, 354, 355,  355,  356,
    357,  357,  358,  358,  359,  359,  360,  360,  361,  361,  362,  363,  363, 364, 364, 365, 365, 366, 366, 367,  368,  368,
    369,  369,  370,  370,  371,  371,  372,  373,  373,  374,  374,  375,  375, 376, 377, 377, 378, 378, 379, 379,  380,  381,
    381,  382,  382,  383,  383,  384,  385,  385,  386,  386,  387,  388,  388, 389, 389, 390, 391, 391, 392, 392,  393,  394,
    394,  395,  395,  396,  397,  397,  398,  398,  399,  400,  400,  401,  401, 402, 403, 403, 404, 404, 405, 406,  406,  407,
    408,  408,  409,  409,  410,  411,  411,  412,  412,  413,  414,  414,  415, 416, 416, 417, 418, 418, 419, 419,  420,  421,
    421,  422,  423,  423,  424,  425,  425,  426,  427,  427,  428,  428,  429, 430, 430, 431, 432, 432, 433, 434,  434,  435,
    436,  436,  437,  438,  438,  439,  440,  440,  441,  442,  442,  443,  444, 444, 445, 446, 446, 447, 448, 448,  449,  450,
    450,  451,  452,  453,  453,  454,  455,  455,  456,  457,  457,  458,  459, 459, 460, 461, 462, 462, 463, 464,  464,  465,
    466,  466,  467,  468,  469,  469,  470,  471,  471,  472,  473,  474,  474, 475, 476, 477, 477, 478, 479, 479,  480,  481,
    482,  482,  483,  484,  485,  485,  486,  487,  488,  488,  489,  490,  490, 491, 492, 493, 493, 494, 495, 496,  496,  497,
    498,  499,  499,  500,  501,  502,  503,  503,  504,  505,  506,  506,  507, 508, 509, 509, 510, 511, 512, 513,  513,  514,
    515,  516,  516,  517,  518,  519,  520,  520,  521,  522,  523,  524,  524, 525, 526, 527, 528, 528, 529, 530,  531,  532,
    532,  533,  534,  535,  536,  536,  537,  538,  539,  540,  541,  541,  542, 543, 544, 545, 545, 546, 547, 548,  549,  550,
    550,  551,  552,  553,  554,  555,  556,  556,  557,  558,  559,  560,  561, 561, 562, 563, 564, 565, 566, 567,  567,  568,
    569,  570,  571,  572,  573,  574,  574,  575,  576,  577,  578,  579,  580, 581, 581, 582, 583, 584, 585, 586,  587,  588,
    589,  589,  590,  591,  592,  593,  594,  595,  596,  597,  598,  598,  599, 600, 601, 602, 603, 604, 605, 606,  607,  608,
    609,  609,  610,  611,  612,  613,  614,  615,  616,  617,  618,  619,  620, 621, 622, 623, 624, 624, 625, 626,  627,  628,
    629,  630,  631,  632,  633,  634,  635,  636,  637,  638,  639,  640,  641, 642, 643, 644, 645, 646, 647, 648,  649,  650,
    651,  652,  653,  654,  655,  656,  657,  658,  659,  660,  661,  662,  663, 664, 665, 666, 667, 668, 669, 670,  671,  672,
    673,  674,  675,  676,  677,  678,  679,  680,  681,  682,  683,  684,  685, 686, 687, 688, 689, 690, 691, 692,  693,  694,
    696,  697,  698,  699,  700,  701,  702,  703,  704,  705,  706,  707,  708, 709, 711, 712, 713, 714, 715, 716,  717,  718,
    719,  720,  721,  722,  724,  725,  726,  727,  728,  729,  730,  731,  732, 734, 735, 736, 737, 738, 739, 740,  741,  742,
    744,  745,  746,  747,  748,  749,  750,  752,  753,  754,  755,  756,  757, 758, 760, 761, 762, 763, 764, 765,  767,  768,
    769,  770,  771,  772,  774,  775,  776,  777,  778,  779,  781,  782,  783, 784, 785, 787, 788, 789, 790, 791,  793,  794,
    795,  796,  797,  799,  800,  801,  802,  804,  805,  806,  807,  808,  810, 811, 812, 813, 815, 816, 817, 818,  820,  821,
    822,  823,  825,  826,  827,  828,  830,  831,  832,  833,  835,  836,  837, 838, 840, 841, 842, 844, 845, 846,  847,  849,
    850,  851,  853,  854,  855,  856,  858,  859,  860,  862,  863,  864,  866, 867, 868, 870, 871, 872, 874, 875,  876,  878,
    879,  880,  882,  883,  884,  886,  887,  888,  890,  891,  892,  894,  895, 896, 898, 899, 900, 902, 903, 905,  906,  907,
    909,  910,  911,  913,  914,  916,  917,  918,  920,  921,  923,  924,  925, 927, 928, 930, 931, 932, 934, 935,  937,  938,
    940,  941,  942,  944,  945,  947,  948,  950,  951,  953,  954,  955,  957, 958, 960, 961, 963, 964, 966, 967,  969,  970,
    972,  973,  974,  976,  977,  979,  980,  982,  983,  985,  986,  988,  989, 991, 992, 994, 995, 997, 998, 1000, 1001, 1003,
    1005, 1006, 1008, 1009, 1011, 1012, 1014, 1015, 1017, 1018, 1020, 1021, 1023
};

void demo_color_light_init(void)
{
    // bl61x_get_chip_temp_init();
    demo_color_light_task();
}

void demo_color_set_param(uint32_t p_Rduty, uint32_t p_Gduty, uint32_t p_Bduty, uint32_t p_Cduty, uint32_t p_Wduty)
{
    SM2235EGH_Set_Color(p_Rduty, p_Gduty, p_Bduty, p_Cduty, p_Wduty);
}

void set_level(uint8_t currLevel)
{
    printf("%s\r\n", __func__);
}

void set_color_red(void)
{
    Rduty = 0xbe8;
    Gduty = 0;
    Bduty = 0;
    Cduty = 0;
    Wduty = 0;
    printf("Rduty=%lx,Gduty=%lx,Bduty=%lx\r\n", Rduty, Gduty, Bduty);
    SM2235EGH_Set_Color(1023, 0, 0, 0, 0);
}

void set_color_green(void)
{
    Rduty = 0;
    Gduty = 0xbe8;
    Bduty = 0;
    Cduty = 0;
    Wduty = 0;
    printf("Rduty=%lx,Gduty=%lx,Bduty=%lx\r\n", Rduty, Gduty, Bduty);
    SM2235EGH_Set_Color(0, 1023, 0, 0, 0);
}
void set_color_blue(void)
{
    Rduty = 0;
    Gduty = 0;
    Bduty = 0xbe8;
    Cduty = 0;
    Wduty = 0;
    printf("Rduty=%lx,Gduty=%lx,Bduty=%lx\r\n", Rduty, Gduty, Bduty);
    SM2235EGH_Set_Color(0, 0, 1023, 0, 0);
}

int get_curve_value(uint16_t temp)
{
    int value = 0;
    temp      = (float) (987 * temp / 254);
    printf("temp %d\r\n", temp);
    if (temp == 0)
    {
        return value;
    }
    if (temp <= 10)
    {
        value = 1;
        return value;
    }
    if (temp == 1023)
    {
        value = 0xbe8;
        return value;
    }
    while (1)
    {
        if ((pwm_curve[value] <= temp) && (pwm_curve[value + 1] >= temp))
        {
            break;
        }
        value++;
    }
    return value;
}
int get_curve_value_forcolor(uint16_t temp)
{
    int value = 0;
    printf("temp %d\r\n", temp);
    if (temp == 0)
    {
        return value;
    }
    if (temp <= 10)
    {
        value = 1;
        return value;
    }
    if (temp == 1023)
    {
        value = 0xbe8;
        return value;
    }
    while (1)
    {
        if ((pwm_curve[value] <= temp) && (pwm_curve[value + 1] >= temp))
        {
            break;
        }
        value++;
    }
    return value;
}
void HSVtoRGB(uint32_t * ofR, uint32_t * ofG, uint32_t * ofB, uint16_t currLevel, uint16_t currHue, uint16_t currSat)
{
    printf("currLevel =%d currHue =%d currSat =%d\r\n", currLevel, currHue, currSat);
    float fH = (float) (currHue);
    float fS = (float) (currSat);
    float fV = (float) (currLevel);
    printf("currLevel =%f currHue =%f currSat =%f\r\n", fV, fH, fV);

    fH = (float) (fH * 360 / 254);
    fS = (float) (fS / 254);
    fV = (float) (fV / 254);

    printf("%f ,%f ,%f\r\n", fH, fS, fV);

    float fC;
    float fHPrime;
    float fX;
    float fM;
    float fR, fG, fB;

    fC      = fV * fS; // Chroma
    fHPrime = fmod(fH / 60.0, 6);
    fX      = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
    fM      = fV - fC;

    if (0 <= fHPrime && fHPrime < 1)
    {
        fR = fC;
        fG = fX;
        fB = 0;
    }
    else if (1 <= fHPrime && fHPrime < 2)
    {
        fR = fX;
        fG = fC;
        fB = 0;
    }
    else if (2 <= fHPrime && fHPrime < 3)
    {
        fR = 0;
        fG = fC;
        fB = fX;
    }
    else if (3 <= fHPrime && fHPrime < 4)
    {
        fR = 0;
        fG = fX;
        fB = fC;
    }
    else if (4 <= fHPrime && fHPrime < 5)
    {
        fR = fX;
        fG = 0;
        fB = fC;
    }
    else if (5 <= fHPrime && fHPrime < 6)
    {
        fR = fC;
        fG = 0;
        fB = fX;
    }
    else
    {
        fR = 0;
        fG = 0;
        fB = 0;
    }

    fR += fM;
    fG += fM;
    fB += fM;
    printf("fR %f ,fG %f ,fB %f\r\n", fR, fG, fB);
    *ofR = fR * 254;
    *ofG = fG * 254;
    *ofB = fB * 254;
}
void set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat)
{
    printf("%s\r\n", __func__);
    uint32_t red, green, blue;

    HSVtoRGB(&red, &green, &blue, currLevel, currHue, currSat);

    if ((currSat = 254) && (currHue == 169)) // blue
    {
        red   = 0;
        green = 0;
        blue  = currLevel;
    }
    if ((currSat = 254) && (currHue == 84)) // green
    {
        red   = 0;
        green = currLevel;
        blue  = 0;
    }

    printf("R %ld G %ld B %ld \r\n", red, green, blue);
    if (currLevel != 0)
    {

        float RGB_total = (float) (red + green + blue);
        float ofR       = (float) (red);
        float ofG       = (float) (green);
        float ofB       = (float) (blue);

        ofR = red * currLevel / (254 * RGB_total);
        ofG = green * currLevel / (254 * RGB_total);
        ofB = blue * currLevel / (254 * RGB_total);
        printf("R %f G %f B %f \r\n", ofR, ofG, ofB);

        red   = ofR * 1023;
        green = ofG * 1023;
        blue  = ofB * 1023;
        printf("R %ld G %ld B %ld  \r\n", red, green, blue);

        new_Rduty = get_curve_value_forcolor(red);
        new_Gduty = get_curve_value_forcolor(green);
        new_Bduty = get_curve_value_forcolor(blue);
    }
    else
    {
        new_Rduty = 0;
        new_Gduty = 0;
        new_Bduty = 0;
    }

    printf("now_Rduty update=%lx,now_Gduty update =%lx,now_Bduty update     =%lx\r\n", new_Rduty, new_Gduty, new_Bduty);
    
    if(lastlevel!=currLevel)
    {
        lastlevel=currLevel;
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
    else
    {
        Cduty = 0;
        Wduty = 0;
        Rduty=new_Rduty;
        Gduty=new_Gduty;
        Bduty=new_Bduty;
        demo_color_set_param(pwm_curve[Rduty], pwm_curve[Gduty], pwm_curve[Bduty], 0, 0);
    }

}

void hw_set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat)
{
    printf("%s\r\n", __func__);

    uint32_t red, green, blue;
    HSVtoRGB(&red, &green, &blue, currLevel, currHue, currSat);

    if ((currSat = 254) && (currHue == 169)) // blue
    {
        red   = 0;
        green = 0;
        blue  = currLevel;
    }
    if ((currSat = 254) && (currHue == 84)) // green
    {
        red   = 0;
        green = currLevel;
        blue  = 0;
    }
    printf("R %ld G %ld B %ld \r\n", red, green, blue);
    if (currLevel != 0)
    {

        float RGB_total = (float) (red + green + blue);
        float ofR       = (float) (red);
        float ofG       = (float) (green);
        float ofB       = (float) (blue);

        ofR = red * currLevel / (254 * RGB_total);
        ofG = green * currLevel / (254 * RGB_total);
        ofB = blue * currLevel / (254 * RGB_total);
        printf("R %f G %f B %f \r\n", ofR, ofG, ofB);

        red   = ofR * 1023;
        green = ofG * 1023;
        blue  = ofB * 1023;
        printf("R %ld G %ld B %ld  \r\n", red, green, blue);
        Rduty = get_curve_value_forcolor(red);
        Gduty = get_curve_value_forcolor(green);
        Bduty = get_curve_value_forcolor(blue);
    }
    else
    {
        Rduty = 0;
        Gduty = 0;
        Bduty = 0;
    }

    Cduty = 0;
    Wduty = 0;
    demo_color_set_param(pwm_curve[Rduty], pwm_curve[Gduty], pwm_curve[Bduty], 0, 0);
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

        warm = (float) (warm * currLevel / 254);
        clod = (float) (clod * currLevel / 254);

        new_Wduty = get_curve_value(warm);
        new_Cduty = get_curve_value(clod);
        printf("now_Cduty update=%lx,now_Wduty update =%lx\r\n", new_Cduty, new_Wduty);

        if(lastlevel!=currLevel)
        {
            lastlevel=currLevel;
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
        else
        {
            Wduty = new_Wduty;
            Cduty = new_Cduty;
            Rduty = 0;
            Gduty = 0;
            Bduty = 0;
            demo_color_set_param(0, 0, 0, pwm_curve[Cduty], pwm_curve[Wduty]);
        }
   
    }
}

void hw_set_temperature(uint8_t currLevel, uint16_t temperature)
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

        warm = (float) (warm * currLevel / 254);
        clod = (float) (clod * currLevel / 254);

        Wduty = get_curve_value(warm);
        Cduty = get_curve_value(clod);
        printf("now_Cduty update=%lx,now_Wduty update =%lx\r\n", Cduty, Wduty);
        Rduty = 0;
        Gduty = 0;
        Bduty = 0;
        demo_color_set_param(0, 0, 0, pwm_curve[Cduty], pwm_curve[Wduty]);
    }
}

void set_warm_temperature(void)
{
    Rduty = 0;
    Gduty = 0;
    Bduty = 0;
    Cduty = 0;
    Wduty = 0xbe8;
    SM2235EGH_Set_Color(0, 0, 0, 0, 987);
}

void set_cold_temperature(void)
{
    Rduty = 0;
    Gduty = 0;
    Bduty = 0;
    Cduty = 0xbe8;
    Wduty = 0;
    SM2235EGH_Set_Color(0, 0, 0, 987, 0);
}

static void Light_TimerHandler(TimerHandle_t p_timerhdl)
{

    if (Light_Timer_Status == 1)
    {
        if ((new_Rduty != Rduty) || (new_Gduty != Gduty) || (new_Bduty != Bduty))
        {
            if ((Cduty != 0) || (Wduty != 0))
            {
                Rduty = new_Rduty;
                Gduty = new_Gduty;
                Bduty = new_Bduty;
            }
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
            Cduty = 0;
            Wduty = 0;
            demo_color_set_param(pwm_curve[Rduty], pwm_curve[Gduty], pwm_curve[Bduty], 0, 0);
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
                if (new_Wduty >= 0xbe8)
                {
                    Wduty = 0xbe8;
                }
                else
                {
                    Wduty = new_Wduty;
                }

                if (new_Cduty >= 0xbe8)
                {
                    Cduty = 0xbe8;
                }
                else
                {
                    Cduty = new_Cduty;
                }
            }
            if ((Rduty != 0) || (Gduty != 0) || (Bduty != 0))
            {
                Cduty = new_Cduty;
                Wduty = new_Wduty;
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
                if ((Cduty <= 0xbe8) && (Wduty <= 0xbe8))
                {
                    Cduty = 0;
                    Wduty = 0;
                }
            }
            Rduty = 0;
            Gduty = 0;
            Bduty = 0;
            demo_color_set_param(0, 0, 0, pwm_curve[Cduty], pwm_curve[Wduty]);
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
#if 0
#include "bflb_adc.h"
struct bflb_device_s * adc;
void bl61x_get_chip_temp_init(void)
{

    adc = bflb_device_get_by_name("adc");

    /* adc clock = XCLK / 2 / 32 */
    struct bflb_adc_config_s cfg;
    cfg.clk_div              = ADC_CLK_DIV_32;
    cfg.scan_conv_mode       = false;
    cfg.continuous_conv_mode = false;
    cfg.differential_mode    = false;
    cfg.resolution           = ADC_RESOLUTION_16B;
    cfg.vref                 = ADC_VREF_2P0V;

    struct bflb_adc_channel_s chan;

    chan.pos_chan = ADC_CHANNEL_TSEN_P;
    chan.neg_chan = ADC_CHANNEL_GND;

    bflb_adc_init(adc, &cfg);
    bflb_adc_channel_config(adc, &chan, 1);
    bflb_adc_tsen_init(adc, ADC_TSEN_MOD_INTERNAL_DIODE);
}

void bl61x_get_chip_temp(void)
{

    float average_filter = 0.0;
    average_filter       = bflb_adc_tsen_get_temp(adc);
    printf("temp = %f\r\n", average_filter);
}
#endif