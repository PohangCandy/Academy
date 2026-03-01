#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <algorithm>

int main()
{
    const int NUM_TRAITS = 5;
    const int QUESTIONS_PER_TRAIT = 20;

    std::vector<std::string> names =
    {
        "창의성",
        "성실성",
        "협력성",
        "개방성",
        "정서안정성"
    };

    // signed 합 저장 (-40~40)
    std::vector<double> traitSum(NUM_TRAITS, 0.0);

    std::random_device rd;
    std::mt19937 gen(rd());

    // Likert -2 ~ +2
    std::uniform_int_distribution<> likertDist(-2, 2);

    // 강조: 0=없음,1=가깝다,2=멀다
    std::uniform_int_distribution<> emphasisDist(0, 2);

    for (int t = 0; t < NUM_TRAITS; t++)
    {
        for (int q = 0; q < QUESTIONS_PER_TRAIT; q++)
        {
            int likert = likertDist(gen);
            int emp = emphasisDist(gen);

            double weight = 1.0;

            if (emp == 1)      weight = 1.5; // 가깝다
            else if (emp == 2) weight = 0.5; // 멀다

            traitSum[t] += likert * weight;
        }
    }

    std::cout << "===== 조직 적합도 결과 =====\n";

    for (int i = 0; i < NUM_TRAITS; i++)
    {
        // -40~40 → 0~100
        double score = (traitSum[i] + 40.0) / 80.0 * 100.0;

        // 클램프
        score = std::max(0.0, std::min(100.0, score));

        std::cout << names[i] << " : " << score << "점\n";
    }

    return 0;
}