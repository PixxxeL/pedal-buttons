#pragma once

#include <memory>
#include <string>


namespace core {

class SingleInstance {
public:
    explicit SingleInstance(const std::string& name);
    ~SingleInstance();

    SingleInstance(const SingleInstance&) = delete;
    SingleInstance& operator=(const SingleInstance&) = delete;

    bool acquired() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}
