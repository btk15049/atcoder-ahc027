import math

string = "OP2_P: 0, OP3_P: 171, OP4_P: 6, OP5_P: 71, OP6_P: 298, OP7_P: 9000, OP8_P: 10"
param = {
    key: int(value)
    for key, value in [x.split(": ") for x in string.split(", ")]
}

total = 1000 + sum(param.values())

value_dict = {
    'OP1_P': 1000 - sum([int(math.floor(1000 * seed / total)) for seed in param.values()]),
    'OP2_P': int(math.floor(1000 * param['OP2_P'] / total)),
    'OP3_P': int(math.floor(1000 * param['OP3_P'] / total)),
    'OP4_P': int(math.floor(1000 * param['OP4_P'] / total)),
    'OP5_P': int(math.floor(1000 * param['OP5_P'] / total)),
    'OP6_P': int(math.floor(1000 * param['OP6_P'] / total)),
    'OP7_P': int(math.floor(1000 * param['OP7_P'] / total)),
    'OP8_P': int(math.floor(1000 * param['OP8_P'] / total)),
}

print(value_dict)

for key, value in value_dict.items():
    print(f'''constexpr int {key} =
#ifdef PARAM_{key}
    PARAM_{key}
#else
    {value}
#endif
    ;
'''
    )

