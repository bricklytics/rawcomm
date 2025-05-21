//
// Created by julio-martins on 5/20/25.
//

#ifndef DATAOBSERVER_H
#define DATAOBSERVER_H

#include <functional>
#include <vector>
#include <optional>
#include <mutex>

template <typename T>
class DataObserver {
public:
    using callback = std::function<void(const T&)>;

    void post(const T& value) {
        std::lock_guard lock(mutex_);
        data_ = value;
        notifyObservers();
    }

    void observe(callback callback) {
        std::lock_guard lock(mutex_);
        observers_.push_back(callback);

        // If data already posted, call observer immediately
        if (data_.has_value()) {
            callback(data_.value());
        }
    }

private:
    void notifyObservers() {
        for (auto& observer : observers_) {
            if (data_.has_value()) {
                observer(data_.value());
            }
        }
    }

    std::vector<callback> observers_;
    std::optional<T> data_;
    std::mutex mutex_;
};

#endif //DATAOBSERVER_H
