#include <node.h>
#include <uv.h>

namespace {

enum class Operation {
    kAdd,
    kSub,
    kMul,
    kDiv,
};

struct OpData {
    OpData(Operation op, double op1, double op2) : op_(op), op1_(op1), op2_(op2) {
    }
    ~OpData() {
        persistent_.Reset();
    }

    Operation op_;
    double op1_;
    double op2_;
    v8::Persistent<v8::Promise::Resolver> persistent_;
};

void LazyCalc(uv_timer_t *handle) {
    uv_timer_stop(handle);
    uv_close(reinterpret_cast<uv_handle_t *>(handle), [](uv_handle_t *h) { delete h; });

    auto isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);

    std::unique_ptr<OpData> data(static_cast<OpData *>(handle->data));
    auto resolver = v8::Local<v8::Promise::Resolver>::New(isolate, data->persistent_);

    double op_result = 0;
    switch (data->op_) {
    case Operation::kAdd:
        op_result = data->op1_ + data->op2_;
        break;
    case Operation::kSub:
        op_result = data->op1_ - data->op2_;
        break;
    case Operation::kMul:
        op_result = data->op1_ * data->op2_;
        break;
    case Operation::kDiv:
        if (data->op2_ == 0) {
            v8::Local<v8::String> msg =
                v8::String::NewFromUtf8(isolate, "division by zero", v8::NewStringType::kNormal).ToLocalChecked();
            resolver->Reject(isolate->GetCurrentContext(), msg);
            return;
        }

        op_result = data->op1_ / data->op2_;
        break;
    }

    v8::Local<v8::Number> ret = v8::Number::New(isolate, op_result);
    resolver->Resolve(isolate->GetCurrentContext(), ret);
}

} // namespace

void OpCommon(const v8::FunctionCallbackInfo<v8::Value> &args, Operation op, int op1, int op2) {
    v8::Isolate *isolate = args.GetIsolate();
    auto resolver = v8::Promise::Resolver::New(isolate->GetCurrentContext()).ToLocalChecked();
    auto promise = resolver->GetPromise();

    auto handle = new uv_timer_t;
    auto data = new OpData(op, op1, op2);
    data->persistent_.Reset(isolate, resolver);
    handle->data = data;
    uv_timer_init(uv_default_loop(), handle);
    uv_timer_start(handle, &LazyCalc, 0, 0);

    args.GetReturnValue().Set(promise);
}

void Add(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto a = static_cast<double>(args[0].As<v8::Number>()->Value());
    const auto b = static_cast<double>(args[1].As<v8::Number>()->Value());
    OpCommon(args, Operation::kAdd, a, b);
}

void Sub(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto a = static_cast<double>(args[0].As<v8::Number>()->Value());
    const auto b = static_cast<double>(args[1].As<v8::Number>()->Value());
    OpCommon(args, Operation::kSub, a, b);
}

void Mul(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto a = static_cast<double>(args[0].As<v8::Number>()->Value());
    const auto b = static_cast<double>(args[1].As<v8::Number>()->Value());
    OpCommon(args, Operation::kMul, a, b);
}

void Div(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto a = static_cast<double>(args[0].As<v8::Number>()->Value());
    const auto b = static_cast<double>(args[1].As<v8::Number>()->Value());
    OpCommon(args, Operation::kDiv, a, b);
}

void Initialize(v8::Local<v8::Object> exports) {
    NODE_SET_METHOD(exports, "add", Add);
    NODE_SET_METHOD(exports, "sub", Sub);
    NODE_SET_METHOD(exports, "mul", Mul);
    NODE_SET_METHOD(exports, "div", Div);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
