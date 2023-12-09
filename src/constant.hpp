/// @brief 一辺の長さの上限
constexpr int N_UB = 40;

/// @brief 一辺の長さの下限
constexpr int N_LB = 20;

/// @brief 汚れやすさの上限
constexpr int D_UB = 1000;

/// @brief 操作回数の上限
constexpr int L_MAX = 100000;

constexpr double LENGTH_THRESHOLD           = 4.0;
constexpr double LENGTH_PENALTY_COEFFICIENT = 1e7;


constexpr double T_START =
#ifdef PARAM_T_START
    PARAM_T_START
#else
    0.01
#endif
    ;

constexpr double T_END =
#ifdef PARAM_T_END
    PARAM_T_END
#else
    0.001
#endif
    ;

constexpr int OP1_P =
#ifdef PARAM_OP1_P
    PARAM_OP1_P
#else
    272
#endif
    ;

constexpr int OP2_P =
#ifdef PARAM_OP2_P
    PARAM_OP2_P
#else
    1
#endif
    ;

constexpr int OP3_P =
#ifdef PARAM_OP3_P
    PARAM_OP3_P
#else
    161
#endif
    ;

constexpr int OP4_P =
#ifdef PARAM_OP4_P
    PARAM_OP4_P
#else
    2
#endif
    ;

constexpr int OP5_P =
#ifdef PARAM_OP5_P
    PARAM_OP5_P
#else
    8
#endif
    ;

constexpr int OP6_P =
#ifdef PARAM_OP6_P
    PARAM_OP6_P
#else
    32
#endif
    ;

constexpr int OP7_P =
#ifdef PARAM_OP7_P
    PARAM_OP7_P
#else
    510
#endif
    ;

constexpr int OP8_P =
#ifdef PARAM_OP8_P
    PARAM_OP8_P
#else
    14
#endif
    ;
