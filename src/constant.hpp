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
    600
#endif
    ;

constexpr int OP2_P =
#ifdef PARAM_OP2_P
    PARAM_OP2_P
#else
    100
#endif
    ;

constexpr int OP3_P =
#ifdef PARAM_OP3_P
    PARAM_OP3_P
#else
    100
#endif
    ;

constexpr int OP4_P =
#ifdef PARAM_OP4_P
    PARAM_OP4_P
#else
    100
#endif
    ;

constexpr int OP5_P =
#ifdef PARAM_OP5_P
    PARAM_OP5_P
#else
    100
#endif
    ;
