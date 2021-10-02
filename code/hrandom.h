
struct random_series
{
    uint32_t Index;
};

inline random_series
RandomSeed(uint32_t Value)
{
    random_series Series;
    Series.Index = Value;
    return Series;
}

inline uint32_t
NextRandomUInt32(random_series* Series)
{
    uint32_t Result = Series->Index;

    Result ^= Result << 13;
    Result ^= Result >> 17;
    Result ^= Result << 5;

    return Result;
}

inline uint32_t
RandomChoice(random_series* Series, uint32_t ChoiceCount)
{
    uint32_t Result = (NextRandomUInt32(Series) % ChoiceCount);
    return Result;
}

inline float
RandomUnilateral(random_series* Series)
{
    float Divisor = 1.0f / (float)0xFFFFFFFF;
    float Result = Divisor * NextRandomUInt32(Series);
    return Result;
}

inline float
RandomBilateral(random_series* Series)
{
    float Result = 2.0f*RandomUnilateral(Series) - 1;
    return Result;
}

inline float
RandomBetween(random_series* Series, float Min, float Max)
{
    float Result = Lerp(Min, RandomUnilateral(Series), Max);
    return Result;
}

inline int32_t
RandomBetween(random_series* Series, int32_t Min, int32_t Max)
{
    int32_t Result = Min + (int32_t)(NextRandomUInt32(Series)%(Max - Min + 1));
    return Result;
}
