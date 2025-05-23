//
// Created by julio-martins on 5/20/25.
//

#ifndef DATAOBSERVER_H
#define DATAOBSERVER_H

#include <functional>
#include <optional>

template <typename T>
class DataObserver {
public:
    using Callback = std::function<void(const T&)>;

    void post(const T& value) {
        data_ = value;
        if (observer_) {
            observer_.value()(data_.value());
        }
    }

    void observe(Callback callback) {
        observer_ = std::move(callback);
        if (data_) {
            observer_.value()(data_.value());
        }
    }

private:
    std::optional<T> data_;
    std::optional<Callback> observer_;
};

#endif //DATAOBSERVER_H
