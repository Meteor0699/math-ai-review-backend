#include "utils/PasswordUtil.h"

#include <iostream>

int main(int argc, char **argv)
{
    const std::string password = "123456";
    const auto hash = mathai::utils::makePasswordHash(password);
    if (hash.rfind("pbkdf2_sha256$", 0) != 0 ||
        !mathai::utils::verifyPassword(password, hash) ||
        mathai::utils::verifyPassword("wrong-password", hash) ||
        !mathai::utils::verifyPassword(
            password,
            "sha256$math-ai-review$b9ef3da9ebe71810e3c5d67b311bf23ce15f70646de275e131db945ed7ff0555"))
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
