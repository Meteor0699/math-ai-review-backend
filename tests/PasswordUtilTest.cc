#include "utils/PasswordUtil.h"

#include <iostream>

int main(int argc, char **argv)
{
    const std::string password = "unit-test-password";
    const auto hash = mathai::utils::makePasswordHash(password);
    if (hash.rfind("pbkdf2_sha256$", 0) != 0 ||
        !mathai::utils::verifyPassword(password, hash) ||
        mathai::utils::verifyPassword("wrong-password", hash) ||
        !mathai::utils::verifyPassword(
            password,
            "sha256$math-ai-review$f321a197ca7da6e57b8bc1a5f843c60dc4c40bd4e0627153e63b16bc8c115ab4"))
    {
        std::cerr << "password hash verification failed\n";
        return 1;
    }
    if (argc == 2 && !mathai::utils::verifyPassword(password, argv[1]))
    {
        std::cerr << "external password hash verification failed\n";
        return 1;
    }
    return 0;
}
