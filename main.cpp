/**
 * @author github.com/rainbow2013 & Qwen3
 * @name funck 0.1
 * @date 2025/9/26 (or 26/9/2025?)
 */
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <utility>


class Expr {
public:
    virtual ~Expr() = default;

    virtual void print(std::ostream &out) const = 0;

    [[nodiscard]] virtual std::shared_ptr<Expr> clone() const = 0;


    static std::shared_ptr<Expr> var(const std::string &name);

    static std::shared_ptr<Expr> lambda(const std::string &param, const std::shared_ptr<Expr> &body);

    static std::shared_ptr<Expr> app(const std::shared_ptr<Expr> &func, const std::shared_ptr<Expr> &arg);
};


class Var final : public Expr {
public:
    std::string name;

    explicit Var(std::string n) : name(std::move(n)) {
    }

    void print(std::ostream &out) const override {
        out << name;
    }

    [[nodiscard]] std::shared_ptr<Expr> clone() const override {
        return std::make_shared<Var>(name);
    }
};


class Lambda final : public Expr {
public:
    std::string param;
    std::shared_ptr<Expr> body;

    Lambda(std::string p, const std::shared_ptr<Expr> &b) : param(std::move(p)), body(b) {
    }

    void print(std::ostream &out) const override {
        out << "% " << param << " . ";
        body->print(out);
    }

    [[nodiscard]] std::shared_ptr<Expr> clone() const override {
        return std::make_shared<Lambda>(param, body->clone());
    }
};


class App final : public Expr {
public:
    std::shared_ptr<Expr> func, arg;

    App(const std::shared_ptr<Expr> &f, const std::shared_ptr<Expr> &a) : func(f), arg(a) {
    }

    void print(std::ostream &out) const override {
        out << "<";
        func->print(out);
        out << " + ";
        arg->print(out);
        out << ">";
    }

    [[nodiscard]] std::shared_ptr<Expr> clone() const override {
        return std::make_shared<App>(func->clone(), arg->clone());
    }
};


std::shared_ptr<Expr> Expr::var(const std::string &name) {
    return std::make_shared<Var>(name);
}

std::shared_ptr<Expr> Expr::lambda(const std::string &param, const std::shared_ptr<Expr> &body) {
    return std::make_shared<Lambda>(param, body);
}

std::shared_ptr<Expr> Expr::app(const std::shared_ptr<Expr> &func, const std::shared_ptr<Expr> &arg) {
    return std::make_shared<App>(func, arg);
}


std::ostream &operator<<(std::ostream &out, const std::shared_ptr<Expr> &expr) {
    if (expr) expr->print(out);
    else out << "<null>";
    return out;
}


static std::map<std::string, int> name_counter;


std::string fresh_name(const std::string &base) {
    int &cnt = name_counter[base];
    cnt++;
    return base + std::to_string(cnt);
}

std::shared_ptr<Expr> alpha_rename(
    const std::shared_ptr<Expr> &expr,
    const std::string &old_name,
    const std::string &new_name,
    std::set<std::string> bound_vars = {}
);

std::string::const_iterator skip_space(std::string::const_iterator it, std::string::const_iterator end) {
    while (it != end && (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r')) ++it;
    return it;
}

std::pair<std::shared_ptr<Expr>, std::string::const_iterator> parse_var(std::string::const_iterator it,
                                                                        std::string::const_iterator end) {
    it = skip_space(it, end);
    if (it == end || (!isalpha(*it) && *it != '_')) return {nullptr, it};

    const auto start = it;
    while (it != end && (isalnum(*it) || *it == '_')) ++it;
    const std::string name(start, it);

    return {Expr::var(name), it};
}


std::pair<std::shared_ptr<Expr>, std::string::const_iterator> parse_lambda(
    std::string::const_iterator it, std::string::const_iterator end);

std::pair<std::shared_ptr<Expr>, std::string::const_iterator> parse_app(std::string::const_iterator it,
                                                                        std::string::const_iterator end);

std::pair<std::shared_ptr<Expr>, std::string::const_iterator> parse_expr(
    std::string::const_iterator it, std::string::const_iterator end);

std::shared_ptr<Expr> substitute(const std::shared_ptr<Expr> &expr,
                                 const std::string &var,
                                 const std::shared_ptr<Expr> &arg);

std::shared_ptr<Expr> eval(const std::shared_ptr<Expr> &expr,
                           const std::map<std::string, std::shared_ptr<Expr> > &env);


std::pair<std::shared_ptr<Expr>, std::string::const_iterator> parse_lambda(
    std::string::const_iterator it, std::string::const_iterator end) {
    auto save = it;
    it = skip_space(it, end);
    if (it == end || *it != '%') return {nullptr, save};

    ++it;
    const auto param_result = parse_var(it, end);
    if (!param_result.first) return {nullptr, save};
    it = param_result.second;

    it = skip_space(it, end);
    if (it == end || *it != '.') return {nullptr, save};
    ++it;

    const auto body_result = parse_expr(it, end);
    if (!body_result.first) return {nullptr, save};

    const auto lambda_body = body_result.first;
    auto body_end = body_result.second;

    std::string param_name;
    if (const auto var_ptr = std::dynamic_pointer_cast<Var>(param_result.first)) {
        param_name = var_ptr->name;
    } else {
        return {nullptr, save};
    }

    return {Expr::lambda(param_name, lambda_body), body_end};
}

std::pair<std::shared_ptr<Expr>, std::string::const_iterator> parse_app(std::string::const_iterator it,
                                                                        std::string::const_iterator end) {
    auto save = it;
    it = skip_space(it, end);
    if (it == end || *it != '<') return {nullptr, save};
    ++it;

    const auto func_result = parse_expr(it, end);
    if (!func_result.first) return {nullptr, save};
    it = func_result.second;

    it = skip_space(it, end);
    if (it == end || *it != '+') return {nullptr, save};
    ++it;

    const auto arg_result = parse_expr(it, end);
    if (!arg_result.first) return {nullptr, save};
    it = arg_result.second;

    it = skip_space(it, end);
    if (it == end || *it != '>') return {nullptr, save};
    ++it;

    return {Expr::app(func_result.first, arg_result.first), it};
}


std::pair<std::shared_ptr<Expr>, std::string::const_iterator> parse_expr(
    std::string::const_iterator it, std::string::const_iterator end) {
    it = skip_space(it, end);
    auto save = it;

    if (it == end) return {nullptr, save};

    if (*it == '%') {
        return parse_lambda(it, end);
    }
    if (*it == '<') {
        return parse_app(it, end);
    }

    return parse_var(it, end);
}

void collect_free_vars(const std::shared_ptr<Expr> &expr, std::set<std::string> &free_vars,
                       std::set<std::string> bound_vars = {}) {
    if (!expr) return;

    if (const auto var_expr = std::dynamic_pointer_cast<Var>(expr)) {
        if (bound_vars.find(var_expr->name) == bound_vars.end()) {
            free_vars.insert(var_expr->name);
        }
        return;
    }

    if (const auto lambda = std::dynamic_pointer_cast<Lambda>(expr)) {
        const std::string param = lambda->param;
        bound_vars.insert(param);
        collect_free_vars(lambda->body, free_vars, bound_vars);
        bound_vars.erase(param);
        return;
    }

    if (const auto app = std::dynamic_pointer_cast<App>(expr)) {
        collect_free_vars(app->func, free_vars, bound_vars);
        collect_free_vars(app->arg, free_vars, bound_vars);
    }
}

std::shared_ptr<Expr> substitute(const std::shared_ptr<Expr> &expr,
                                 const std::string &var,
                                 const std::shared_ptr<Expr> &arg) {
    if (!expr) return nullptr;


    if (const auto var_expr = std::dynamic_pointer_cast<Var>(expr)) {
        return var_expr->name == var ? arg->clone() : expr->clone();
    }


    if (const auto lambda = std::dynamic_pointer_cast<Lambda>(expr)) {
        const std::string param = lambda->param;
        const auto body = lambda->body;

        if (param == var) {
            return expr->clone();
        }


        std::set<std::string> free_in_arg;

        collect_free_vars(arg, free_in_arg);

        if (free_in_arg.find(param) != free_in_arg.end()) {
            std::string new_param = fresh_name(param);
            const auto renamed_body = substitute(body, param, std::make_shared<Var>(new_param));
            const auto new_lambda = Expr::lambda(new_param, renamed_body);

            return substitute(new_lambda, var, arg);
        }


        const auto new_body = substitute(body, var, arg);
        return Expr::lambda(param, new_body);
    }


    if (const auto app = std::dynamic_pointer_cast<App>(expr)) {
        const auto new_func = substitute(app->func, var, arg);
        const auto new_arg = substitute(app->arg, var, arg);
        return Expr::app(new_func, new_arg);
    }

    return expr->clone();
}

std::shared_ptr<Expr> eval(const std::shared_ptr<Expr> &expr,
                           const std::map<std::string, std::shared_ptr<Expr> > &env) {
    if (!expr) return nullptr;


    if (const auto var_expr = std::dynamic_pointer_cast<Var>(expr)) {
        if (const auto it = env.find(var_expr->name); it != env.end()) {
            return eval(it->second->clone(), env);
        }
        return expr->clone();
    }


    if (std::dynamic_pointer_cast<Lambda>(expr)) {
        return expr->clone();
    }


    if (auto app = std::dynamic_pointer_cast<App>(expr)) {
        const auto evaluated_func = eval(app->func, env);
        const auto evaluated_arg = eval(app->arg, env);


        if (const auto lambda = std::dynamic_pointer_cast<Lambda>(evaluated_func)) {
            const std::string param = lambda->param;
            const auto body = lambda->body;


            const auto result = substitute(body, param, evaluated_arg);


            return eval(result, env);
        }


        return Expr::app(evaluated_func, evaluated_arg);
    }

    return expr->clone();
}


int main() {
    std::map<std::string, std::shared_ptr<Expr> > env;

    std::string line;
    std::cout << "[funck] Ready.\n";


    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;


        const auto eq = line.find('=');
        if (eq != std::string::npos) {
            std::string name = line.substr(0, eq);

            name.erase(name.find_last_not_of(" \t\n\r") + 1);

            std::string expr_str = line.substr(eq + 1);
            auto it = expr_str.begin();


            if (auto result = parse_expr(it, expr_str.end()); result.first && result.second == expr_str.end()) {
                env[name] = result.first;
                std::cout << "[funck] Bound: " << name << "\n";
            } else {
                std::cout << "[funck] Parse error in binding.\n";
            }
            continue;
        }
        if (auto result = parse_expr(line.begin(), line.end()); result.first && result.second == line.end()) {
            auto evaluated = eval(result.first, env);
            std::cout << "| " << evaluated << " |\n";
        } else {
            std::cout << "[funck] Parse error.\n";
        }
    }
    return 0;
}
