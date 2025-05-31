#include "ProgressUtils.hpp"

void SimpleProgressBar::print() const
{
    const float percentage = (static_cast<float>(current) / total);
    const int filled = static_cast<int>(width * percentage);

    std::cout << "\r[";
    for (int i = 0; i < width; ++i)
    {
        if (i < filled)
            std::cout << completeChar;
        else
            std::cout << incompleteChar;
    }
    std::cout << "] " << static_cast<int>(percentage * 100) << "% ";
    std::cout.flush();
}
