/// @brief 一辺の長さの上限
constexpr int N_UB = 40;

/// @brief 一辺の長さの下限
constexpr int N_LB = 20;

/// @brief 汚れやすさの上限
constexpr int D_UB = 1000;

/// @brief 操作回数の上限
constexpr int L_MAX = 100000;


constexpr double LENGTH_THRESHOLD =
#ifdef PARAM_LENGTH_THRESHOLD
    PARAM_LENGTH_THRESHOLD
#else
    50.0
#endif
    ;


constexpr double LENGTH_PENALTY_COEFFICIENT =
#ifdef PARAM_LENGTH_PENALTY_COEFFICIENT
    PARAM_LENGTH_PENALTY_COEFFICIENT
#else
    1e9
#endif
    ;


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

constexpr int OP1_LEN_MAX =
#ifdef PARAM_OP1_LEN_MAX
    PARAM_OP1_LEN_MAX
#else
    10
#endif
    ;

constexpr int OP3_MAX_TRIES =
#ifdef PARAM_OP3_MAX_TRIES
    PARAM_OP3_MAX_TRIES
#else
    25
#endif
    ;

constexpr double OP3_LEN_MAX_COEF =
#ifdef PARAM_OP3_LEN_MAX_COEF
    PARAM_OP3_LEN_MAX_COEF
#else
    0.5
#endif
    ;

constexpr int OP3_LEN_MIN =
#ifdef PARAM_OP3_LEN_MIN
    PARAM_OP3_LEN_MIN
#else
    16
#endif
    ;

constexpr int OP6_LEN_MAX =
#ifdef PARAM_OP6_LEN_MAX
    PARAM_OP6_LEN_MAX
#else
    10
#endif
    ;

constexpr int OP7_LEN_MAX =
#ifdef PARAM_OP7_LEN_MAX
    PARAM_OP7_LEN_MAX
#else
    10
#endif
    ;

constexpr int OP7_DIST_THRESHOLD =
#ifdef PARAM_OP7_DIST_THRESHOLD
    PARAM_OP7_DIST_THRESHOLD
#else
    5
#endif
    ;

constexpr int OP1_P =
#ifdef PARAM_OP1_P
    PARAM_OP1_P
#else
    240
#endif
    ;

constexpr int OP2_P =
#ifdef PARAM_OP2_P
    PARAM_OP2_P
#else
    0
#endif
    ;

constexpr int OP3_P =
#ifdef PARAM_OP3_P
    PARAM_OP3_P
#else
    60
#endif
    ;

constexpr int OP4_P =
#ifdef PARAM_OP4_P
    PARAM_OP4_P
#else
    0
#endif
    ;

constexpr int OP5_P =
#ifdef PARAM_OP5_P
    PARAM_OP5_P
#else
    7
#endif
    ;

constexpr int OP6_P =
#ifdef PARAM_OP6_P
    PARAM_OP6_P
#else
    29
#endif
    ;

constexpr int OP7_P =
#ifdef PARAM_OP7_P
    PARAM_OP7_P
#else
    647
#endif
    ;

constexpr int OP8_P =
#ifdef PARAM_OP8_P
    PARAM_OP8_P
#else
    17
#endif
    ;
