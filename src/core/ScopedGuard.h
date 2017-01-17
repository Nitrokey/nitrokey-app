//
// Created by sz on 17.01.17.
//

#ifndef NITROKEYAPP_SCOPEDGUARD_H
#define NITROKEYAPP_SCOPEDGUARD_H
#include <functional>

class ScopedGuard{
public:
    explicit ScopedGuard(const std::function<void()> &function_to_run) : function_to_run(function_to_run), cancel_action(false) {}
    ~ScopedGuard(){
        if (!cancel_action){
            function_to_run();
        }
    }
    void cancel(){
        cancel_action = true;
    }
    ScopedGuard(const ScopedGuard&) = delete;
    ScopedGuard& operator=(const ScopedGuard &) = delete;
private:
    std::function<void()> function_to_run;
    bool cancel_action;
};

#endif //NITROKEYAPP_SCOPEDGUARD_H
