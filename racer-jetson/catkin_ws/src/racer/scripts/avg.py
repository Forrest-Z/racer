import numpy as np

data = [
    16049.7337474,
    15581.3590584,
    14908.7476233,
    15886.5245963,
    14859.5341514,
    15301.6510701,
    16027.2427974,
    14716.558643,
    15338.0077239,
    15273.7901655,
    15926.7171024,
    15222.9013872,
    15469.2537628,
    15254.9202445,
    13717.3541208,
    17128.8189956,
    15408.7255085,
    15252.8068991,
    15465.3606461,
    14706.2811234,
    16003.6451669,
    14827.9647102,
    15161.700692,
    15486.7516879,
    15537.1081489,
    14730.6391946,
    16255.192116,
    14600.1895474,
    15760.2543197,
    14714.4340113,
    15364.7336012,
    15085.7017584,
    16166.442664,
    15471.1557648,
    15527.5216247,
    14990.5346283,
    15334.6714652,
    15060.251687,
    15956.6876612,
    15432.8438267,
    15217.4075695,
    15666.5333855,
    15343.4107933,
    14673.1682148,
    15369.6489146,
    15853.1877451,
    15036.3195817,
    15608.7868155,
    14618.9405851,
    15432.4832967,
    15580.1647396,
    15157.3521786,
    15745.2287231,
    14562.390298,
    15936.478237,
    15064.9740879,
    15603.2566873,
    15381.2794393,
    15262.3215678,
    15190.2307114,
    14616.1858544,
    15832.3987561,
    15330.3252981,
    15506.4728562,
    15767.7778772,
    15501.5605916,
    15325.5239852,
    14719.4491268,
    15664.2116058,
    15799.7388247,
    15219.0825688,
    14902.10537,
    15292.205315,
    15974.4371429,
    14807.5736621,
    15340.1183603,
    15628.4504693,
    15288.7251351,
    15149.6179932,
    15319.6433489,
    15321.4363494,
    15288.6366766,
    15524.8760025,
    15177.1144999,
    15565.1136571,
    15463.4299046,
    15340.8275326,
    14900.9911227,
    15495.1066076,
    15548.261655,
    15126.3761313,
    15284.6571032,
    15804.8419164,
    15332.6381144,
    14636.0834726,
    15710.6849315,
    15427.9780936,
    15641.9883503,
    15711.5256595,
    14618.4499406,
    15065.4798943,
    15204.1860813,
    15302.4485896,
    15606.0212615,
    14910.1465829,
    15595.6131332,
    15233.1983582,
    14956.7954919,
    15533.2721191,
    15533.3634312,
    14891.747432,
    14842.4020085,
    15601.0457331,
    15436.6304081,
    15667.2765004,
    15373.0467868,
    15093.0259603,
    15246.9493412,
    15839.3332716,
    15303.8666072,
    15124.7695705,
    15691.9320874,
    15190.6382294,
    15678.245639,
    15199.7840226,
    14722.9362209,
    16076.2189405,
    15191.686233,
    14882.8008201,
    15405.6711092,
    15638.9950463,
    14818.4918872,
    15586.6897894,
    15527.6128691,
    15390.4172588,
    15375.4440845,
    15228.6332096,
    14863.5276412,
    15241.9851872,
    15843.8854045,
    15294.2115621,
    14965.316481,
    15674.7115282,
    14925.4213431,
    15402.7110284,
    15114.8683804,
    14688.8773264,
    15897.4558406,
    15751.9866946,
    15432.4832967,
    15080.1056927,
    15169.6117164,
    15222.2290583,
    15548.6276148,
    15633.9059745,
    15487.8409491,
    15287.2214798,
    14938.030884,
    16130.2270316,
    15140.5902879,
    14812.8363902,
    15688.0192359,
    15032.9200398,
    15463.1885958,
    15440.4188482,
    14924.8187436,
    15661.0454876,
    16130.6582506,
    14448.4371208,
    15440.779749,
    15616.1664204,
    14825.7344676,
    15536.0119471,
    15641.0007419,
    15020.0170526,
    15490.732941,
    14973.6303415,
    15739.7890172,
    14913.7121571,
    15784.2607283,
    15438.1636004,
    14931.8156407,
    15295.7165929,
    15191.686233,
    15782.7522936,
    15445.7137513,
    15040.1359966,
    15045.8452148,
    16058.7676546,
    15246.2194991,
    15204.5360232,
    15305.0189401,
    15772.3896045,
    15285.0107592,
    15454.0545662,
    15516.8534232,
    14961.9772051,
    15316.4334459,
    15709.4893844,
    15010.063015,
    15552.8373916,
    14733.0210256,
    15557.4124299,
    15474.1455702,
    15039.422649,
    15754.3345695,
    15332.549147,
    15698.6443759,
    14938.3311474,
    15587.8299362,
    15293.7394566,
    14845.6881172,
    15455.5912217,
    15311.6704042,
    15750.4844218,
    14861.2891651,
    16167.0756957,
    15043.7037501,
    15707.6030317,
    15024.628821,
    15663.097396,
    15649.8319178,
    15383.8464882,
    15370.3641313,
    14869.6861541,
    15362.8576744,
    15322.1470859,
    15686.0635418,
    15450.9821716,
    15311.7591294,
    15578.235842,
    15216.6188894,
    15830.4069015,
    14346.424035,
    16101.7599493,
    14990.5346283,
    15315.75283,
    15841.5107642,
    14407.018393,
    15666.1618545,
    16127.4704915,
    14747.3068642,
    15538.2958755,
    15466.899551,
    15272.7308037,
    15749.6395194,
    15448.3625649,
    15289.43284,
    15769.9422296,
    14842.4020085,
    16023.3217799,
    15147.7942239,
    15127.0143609,
    15054.5029426,
    15888.2530246,
    14974.1111758,
    15834.7706653,
    15230.6520724,
    15760.630327,
    14506.7589741,
    15611.4611163,
    15283.3605047,
    15855.8513128,
    15145.0867203,
    15288.9905167,
    14921.9344335,
    15502.7428893,
    15518.3114572,
    14848.9756814,
    15569.4241035,
    15226.1761859,
    14455.3106879,
    16204.9497737,
    14583.7273778,
    15368.4868295,
    15628.0807424,
    15482.2147486,
    14751.917372,
    15481.7612009,
    15478.7684521,
    15338.8683977,
    15085.3700239,
    15753.8649385,
    14751.3622509,
    15874.6299,
    14808.1821986,
    15402.3386044,
    15494.0163242,
    15202.261689,
    15417.4461903,
    15299.4985109,
    15143.1933293,
    15592.4842005
]

print(np.mean(data))