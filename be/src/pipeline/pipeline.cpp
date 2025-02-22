// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "pipeline.h"

#include "pipeline_fragment_context.h"

namespace doris::pipeline {

Status Pipeline::prepare(RuntimeState* state) {
    for (auto& op : _operators) {
        RETURN_IF_ERROR(op->prepare(state));
    }
    RETURN_IF_ERROR(_sink->prepare(state));
    return Status::OK();
}

Operators Pipeline::build_operators() {
    Operators operators;
    for (auto& operator_t : _operators) {
        auto o = operator_t->build_operator();
        auto s = o->init(operator_t->exec_node(), _context->get_runtime_state());
        if (!s.ok()) {
            // TODO pipeline 1 返回状态
        }
        operators.emplace_back(std::move(o));
    }
    return operators;
}

void Pipeline::close(RuntimeState* state) {
    Status rt_s = Status::OK();
    for (auto& op : _operators) {
        op->close(state);
    }
    _sink->close(state);
}

Status Pipeline::set_source(OperatorTemplatePtr& source_) {
    if (_source) {
        return Status::InternalError("set source twice");
    }
    if (!_operators.empty()) {
        return Status::InternalError("should set source before other operator");
    }
    if (!source_->is_source()) {
        return Status::InternalError("should set a source operator but {}", typeid(source_).name());
    }
    _source = source_;
    _operators.emplace_back(_source);
    return Status::OK();
}

Status Pipeline::add_operator(OperatorTemplatePtr& op) {
    if (!_source) {
        return Status::InternalError("should set source first");
    }
    if (_sink) {
        return Status::InternalError("should set sink last.");
    }
    _operators.emplace_back(op);
    return Status::OK();
}

Status Pipeline::set_sink(OperatorTemplatePtr& sink_) {
    if (_sink) {
        return Status::InternalError("set sink twice");
    }
    if (!sink_->is_sink()) {
        return Status::InternalError("should set a sink operator but {}", typeid(sink_).name());
    }
    _sink = sink_;
    return Status::OK();
}

} // namespace doris::pipeline