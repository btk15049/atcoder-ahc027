/// @brief 箱の個数
constexpr int N = 200;

/// @brief 山の数
constexpr int M = 10;

/// @brief 操作回数の上限
constexpr int K = 5000;


constexpr double T_START =
#ifdef PARAM_T_START
    PARAM_T_START
#else
    1
#endif
    ;

constexpr double T_END =
#ifdef PARAM_T_END
    PARAM_T_END
#else
    0.0001
#endif
    ;
