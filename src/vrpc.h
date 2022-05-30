#ifndef VRPC_H
#define VRPC_H

#include "notstd.hpp"

#ifdef ARDUINO_ARCH_MEGAAVR
#include <ArduinoSTL.h>
#endif

#ifndef ARDUINO_ARCH_MEGAAVR
#include <ArduinoUniqueID.h>
#endif

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <map>
#include <vector>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace vrpc {

PubSubClient client;

const String compile_date = __DATE__ " " __TIME__;

typedef DynamicJsonDocument Json;

// Singleton helper
template <typename T>
inline T& init() {
  static T t;
  return t;
}

namespace details {

// The code below was formulated as an answer to StackOverflow and can be read
// here:
// http://stackoverflow.com/questions/10766112/c11-i-can-go-from-multiple-args-to-tuple-but-can-i-go-from-tuple-to-multiple

template <typename R,
          typename F,
          typename Tuple,
          bool Done,
          int Total,
          int... N>
struct call_impl {
  static R call(F f, Tuple&& t) {
    return call_impl<R, F, Tuple, Total == 1 + sizeof...(N), Total, N...,
                     sizeof...(N)>::call(f, notstd::forward<Tuple>(t));
  }
};

template <typename R, typename F, typename Tuple, int Total, int... N>
struct call_impl<R, F, Tuple, true, Total, N...> {
  static R call(F f, Tuple t) {
    // FIXME: This should really perfectly forward here, but does not compile
    // yet return f(notstd::get<N>(notstd::forward<Tuple>(t))...);
    return f(notstd::get<N>(t)...);
  }
};
}  // namespace details

/**
 * Call a function f with arguments unpacked from a notstd::tuple
 * @param f The function to be called
 * @param t A tuple containing the function arguments
 */
template <typename R, typename F, typename Tuple>
R call(F f, Tuple&& t) {
  typedef typename notstd::decay<Tuple>::type ttype;
  return details::call_impl<
      R, F, Tuple, 0 == notstd::tuple_size<ttype>::value,
      notstd::tuple_size<ttype>::value>::call(f, notstd::forward<Tuple>(t));
}

// unpack

namespace details {

template <size_t N, typename Ret, typename... Args>
struct unpack_impl;

template <size_t N, typename Ret, typename Arg, typename... Args>
struct unpack_impl<N, Ret, Arg, Args...> {
  template <typename A = Arg, typename R = Ret>
  static void unpack(const Json& j, R& t) {
    typedef typename notstd::decay<A>::type dA;
    notstd::get<N>(t) = j["a"][N].as<dA>();
    unpack_impl<N + 1, Ret, Args...>::unpack(j, t);
  }
};

template <size_t N, typename Ret, typename Arg>
struct unpack_impl<N, Ret, Arg> {
  template <typename A = Arg, typename R = Ret>
  static void unpack(const Json& j, R& t) {
    typedef typename notstd::decay<A>::type dA;
    notstd::get<N>(t) = j["a"][N].as<dA>();
  }
};

template <size_t N, typename Ret>
struct unpack_impl<N, Ret> {
  template <typename R = Ret>
  static void unpack(const Json&, R&) {
    // Do nothing
  }
};

}  // namespace details

template <typename... Args>
notstd::tuple<Args...> unpack(const Json& j) {
  typedef typename notstd::tuple<Args...> Ret;
  Ret t;
  details::unpack_impl<0, Ret, Args...>::unpack(j, t);
  return t;
}

class AbstractFunction {
 public:
  AbstractFunction() = default;

  virtual ~AbstractFunction() = default;

  void call_function(Json& j) { this->do_call_function(j); }

 protected:
  virtual void do_call_function(Json&) = 0;
};

template <typename R, typename... Args>
class GlobalFunction : public AbstractFunction {
  notstd::function<R(Args...)> _f;

 public:
  template <typename F>
  GlobalFunction(F&& f) : _f(f) {}
  virtual ~GlobalFunction() = default;

  virtual void do_call_function(Json& j) {
    j["r"] = call<R>(_f, unpack<Args...>(j));
  }
};

template <typename... Args>
class GlobalFunction<void, Args...> : public AbstractFunction {
  notstd::function<void(Args...)> _f;

 public:
  template <typename F>
  GlobalFunction(F&& f) : _f(f) {}
  virtual ~GlobalFunction() = default;

  virtual void do_call_function(Json& j) {
    call<void>(_f, unpack<Args...>(j));
    j["r"] = nullptr;
  }
};

class Registry {
  friend Registry& init<Registry>();
  typedef std::map<String, AbstractFunction*> StringFunctionMap;
  typedef std::map<String, StringFunctionMap> FunctionRegistry;

  FunctionRegistry _function_registry;

 public:
  template <typename Func, Func f, typename R, typename... Args>
  static void register_global_function(const String& function_name) {
    AbstractFunction* func = new GlobalFunction<R, Args...>(f);
    init<Registry>()._function_registry["__global__"][function_name] = func;
  }

  static String call(const String& jsonString) {
    Json json(256);
    DeserializationError err = deserializeJson(json, jsonString);
    if (err) {
      Serial.print(F("ERROR [VRPC] JSON parsing failed because: "));
      Serial.println(err.c_str());
      return String("{\"e\": \"JSON parsing failed because: ") +
             String(err.c_str()) + String("\"}");
    }
    Registry::call(json);
    String res;
    serializeJson(json, res);
    return res;
  }

  static void call(Json& json) {
    String context = json["c"].as<String>();
    String function_name = json["f"].as<String>();
    // TODO implement support for overloading
    // JsonVariant args = json["data"];
    // function_name += vrpc::get_signature(args);
    auto it_t = init<Registry>()._function_registry.find(context);
    if (it_t != init<Registry>()._function_registry.end()) {
      auto it_f = it_t->second.find(function_name);
      if (it_f != it_t->second.end()) {
        it_f->second->call_function(json);
      } else {
        Serial.print("ERROR [VRPC] Could not find function: ");
        Serial.println(function_name);
        json["e"] = "Could not find function: " + function_name;
      }
    } else {
      Serial.print("ERROR [VRPC] Could not find context: ");
      Serial.println(context);
      json["e"] = "Could not find context: " + context;
    }
  }

  static std::vector<String> get_classes() {
    std::vector<String> classes;
    for (const auto& kv : init<Registry>()._function_registry) {
      classes.push_back(kv.first);
    }
    return classes;
  }

  static std::vector<String> get_static_functions(const String& class_name) {
    std::vector<String> functions;
    const auto& it = init<Registry>()._function_registry.find(class_name);
    if (it != init<Registry>()._function_registry.end()) {
      for (const auto& kv : it->second) {
        functions.push_back(kv.first);
      }
    }
    return functions;
  }
};

template <typename Func, Func f, typename R, typename... Args>
struct GlobalFunctionRegistrar {
  GlobalFunctionRegistrar(const String& function_name) {
    Registry::register_global_function<Func, f, R, Args...>(function_name);
  }
};

template <typename Func, Func f, typename R, typename... Args>
struct RegisterGlobalFunction {
  static const GlobalFunctionRegistrar<Func, f, R, Args...> registerAs;
};

}  // namespace vrpc

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @brief Agent that transforms existing code to be operated from a remote
 * location.
 *
 */
class VrpcAgent {
  String _domain_agent;
  String _token;
  String _username;
  String _broker;
  long _lastReconnect = 0;

 public:
  /**
   * @brief Constructs an agent
   *
   * @param maxBytesPerMessage [optional, default: `1024`] Specifies the maximum
   * size a single MQTT message may have
   */
  VrpcAgent(int maxBytesPerMessage = 1024) {
    vrpc::client.setBufferSize(maxBytesPerMessage);
  }

  /**
   * @brief Initializes the object using a client class for network transport
   *
   * @tparam T Type of the client class
   * @param netClient A client class following the interface as described
   * [here](https://www.arduino.cc/en/Reference/ClientConstructor)
   * @param domain [optional, default: `"vrpc"`] The domain under which
   * the agent-provided code is reachable
   * @param token [optional] Access token as used by Heisenware GmbH or MQTT
   * password if own broker is used
   * @param broker [optional, default: `"vrpc.io"`] Address of the MQTT broker
   * @param username [optional] MQTT username (not needed when using the vrpc.io
   * broker)
   */
  template <typename T>
  void begin(T& netClient,
             const String& domain = "vrpc",
             const String& token = "",
             const String& broker = "vrpc.io",
             const String& username = "") {
    _domain_agent = domain + "/" + VrpcAgent::get_unique_id();
    _token = token == "" ? VrpcAgent::get_id_from_compile_date() : token;
    _username = username == "" ? _domain_agent : username;
    _broker = broker;
    vrpc::client.setClient(netClient);
    vrpc::client.setServer(_broker.c_str(), 1883);
    vrpc::client.setKeepAlive(15);
    vrpc::client.setCallback(on_message);
  }

  /**
   * @brief Reports the current connectivity status
   *
   * @return true when connected, false otherwise
   */
  bool connected() { return vrpc::client.connected(); }

  /**
   * @brief Connect the agent to the broker.
   *
   * The function will try to connect forever. Inspect the serial monitor
   * to see the connectivity progress.
   */
  bool connect() {
    Serial.println("\nConnecting to message broker...");
    Serial.print("domain/agent: ");
    Serial.println(_domain_agent);
    Serial.print("broker: ");
    Serial.println(_broker);
    String clientId = "va3" + VrpcAgent::get_unique_id();
    String willTopic(_domain_agent + "/__agentInfo__");
    String willMessage = this->create_agent_info_payload(false);
    Serial.print("clientId: ");
    Serial.println(clientId);
    bool connected = false;
    if (_token == "" && _username == "") {
      connected = vrpc::client.connect(clientId.c_str(), willTopic.c_str(), 1, true,
                      willMessage.c_str());
    } else {
      connected = vrpc::client.connect(clientId.c_str(), _username.c_str(), _token.c_str(),
                      willTopic.c_str(), 1, true, willMessage.c_str());
    }
    // finish here if we could not connect
    if (!connected) {
      return false;
    }
    // otherwise provide info messages
    Serial.println("[OK]");
    publish_agent_info();
    const auto& classes = vrpc::Registry::get_classes();
    for (const auto& class_name : classes) {
      publish_class_info(class_name);
      const auto& functions = vrpc::Registry::get_static_functions(class_name);
      for (const auto& func : functions) {
        String topic(_domain_agent + "/" + class_name + "/__static__/" + func);
        vrpc::client.subscribe(topic.c_str());
      }
    }
    return vrpc::client.connected();
  }

  /**
   * @brief This function will send and receive VRPC packets
   *
   * **IMPORTANT**: This function should be called in every `loop`
   */
  void loop() {
    if (!vrpc::client.connected()) {
      long now = millis();
      if (now - _lastReconnect > 5000) {
        _lastReconnect = now;
        Serial.print("not connected, because: ");
        Serial.println(get_state());
        if (connect()) {
          _lastReconnect = 0;
        }
      }
    } else {
      vrpc::client.loop();
    }
  }

 private:
  String get_state() {
    switch (vrpc::client.state()) {
      case -4:
        return "no keepalive response";

      case -3:
        return "network connection was broken";

      case -2:
        return "network connection failed";

      case -1:
        return "disconnected cleanly";

      case 1:
        return "mqtt version mismatch";

      case 2:
        return "server rejected client identifier";

      case 3:
        return "server was unable to accept the connection";

      case 4:
        return "username/password were rejected";

      case 5:
        return "client was not authorized to connect";
    }
  }

  static void on_message(char* topic, byte* payload, unsigned int size) {
    String topicString(topic);
    std::vector<String> tokens = VrpcAgent::tokenize(topicString, '/');
    if (tokens.size() != 5) {
      Serial.println("ERROR [VRPC] Received invalid message");
      return;
    }
    String class_name(tokens[2]);
    String instance(tokens[3]);
    String method(tokens[4]);
    vrpc::Json j(1024);
    deserializeJson(j, payload, size);
    j["c"] = instance == "__static__" ? class_name : instance;
    j["f"] = method;
    const String sender = j["s"];
    Serial.print("Going to call: ");
    Serial.println(method);
    vrpc::Registry::call(j);
    j.remove("s");
    j.remove("c");
    j.remove("f");
    String res;
    serializeJson(j, res);
    vrpc::client.publish(sender.c_str(), res.c_str());
  }

  static String get_id_from_compile_date() {
    String id;
    for (size_t i = vrpc::compile_date.length() - 1; i > 2; --i) {
      const char x = vrpc::compile_date[i];
      if (x != ' ' && x != ':')
        id += x;
    }
    return id;
  }

  static String get_unique_id() {
#ifdef ARDUINO_ARCH_MEGAAVR
    String id = "ar-" + VrpcAgent::get_id_from_compile_date();
    return id;
#else
    String id = "ar";
    for (size_t i = 0; i < 8; i++) {
      if (UniqueID8[i] < 0x10) {
        id += "0";
      }
      id += String(UniqueID8[i], HEX);
    }
    return id;
#endif
  }

  void publish_agent_info() {
    String topic(_domain_agent + "/__agentInfo__");
    String json = this->create_agent_info_payload(true);
    Serial.println("Sending AgentInfo...");
    Serial.println(json);
    vrpc::client.publish(topic.c_str(), json.c_str(), true);
  }

  String create_agent_info_payload(bool isOnline) {
    return isOnline ? "{\"status\":\"online\",\"hostname\":\"arduino-board\"}"
                    : "{\"status\":\"offline\",\"hostname\":\"arduino-board\"}";
  }

  void publish_class_info(const String& class_name) {
    String json("{\"className\":\"");
    json += class_name +
            "\",\"instances\":[],\"memberFunctions\":[],\"staticFunctions\":[";
    const auto& functions = vrpc::Registry::get_static_functions(class_name);
    for (const auto& func : functions)
      json += "\"" + func + "\",";
    json[json.length() - 1] = ']';
    json += "}";
    const String topic(_domain_agent + "/" + class_name + "/__classInfo__");
    Serial.println("Sending ClassInfo...");
    Serial.println(json);
    vrpc::client.publish(topic.c_str(), json.c_str(), true);
  }

  static std::vector<String> tokenize(const String& input, char delim) {
    std::vector<String> output;
    size_t pos = 0;
    for (size_t i = 0; i < input.length(); ++i) {
      if (input[i] == delim) {
        output.push_back(input.substring(pos, i));
        pos = i + 1;
      }
    }
    output.push_back(input.substring(pos));
    return output;
  }
};

/*----------------------------- Macro utility
 * --------------------------------*/

#define CAT(A, B) A##B
#define SELECT(NAME, NUM) CAT(_##NAME##_, NUM)
#define GET_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, COUNT, ...) COUNT
#define VA_SIZE(...) GET_COUNT(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define VA_SELECT(NAME, ...) SELECT(NAME, VA_SIZE(__VA_ARGS__))(__VA_ARGS__)

#define VRPC_GLOBAL_FUNCTION(...) VA_SELECT(VRPC_GLOBAL_FUNCTION, __VA_ARGS__)

/*---------------------------- Zero arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_2(Ret, Function)                                 \
  template <>                                                                  \
  const vrpc::GlobalFunctionRegistrar<                                         \
      decltype(static_cast<Ret (*)()>(Function)), &Function, Ret>              \
      vrpc::RegisterGlobalFunction<decltype(static_cast<Ret (*)()>(Function)), \
                                   &Function, Ret>::registerAs(#Function);

/*----------------------------- One argument ---------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_3(Ret, Function, A1)                      \
  template <>                                                           \
  const vrpc::GlobalFunctionRegistrar<                                  \
      decltype(static_cast<Ret (*)(A1)>(Function)), &Function, Ret, A1> \
      vrpc::RegisterGlobalFunction<decltype(static_cast<Ret (*)(A1)>(   \
                                       Function)),                      \
                                   &Function, Ret, A1>::registerAs(#Function);

/*----------------------------- Two arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_4(Ret, Function, A1, A2)                       \
  template <>                                                                \
  const vrpc::GlobalFunctionRegistrar<decltype(static_cast<Ret (*)(A1, A2)>( \
                                          Function)),                        \
                                      &Function, Ret, A1, A2>                \
      vrpc::RegisterGlobalFunction<                                          \
          decltype(static_cast<Ret (*)(A1, A2)>(Function)), &Function, Ret,  \
          A1, A2>::registerAs(#Function);

/*--------------------------- Three arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_5(Ret, Function, A1, A2, A3)                  \
  template <>                                                               \
  const vrpc::GlobalFunctionRegistrar<                                      \
      decltype(static_cast<Ret (*)(A1, A2, A3)>(Function)), &Function, Ret, \
      A1, A2, A3>                                                           \
      vrpc::RegisterGlobalFunction<                                         \
          decltype(static_cast<Ret (*)(A1, A2, A3)>(Function)), &Function,  \
          Ret, A1, A2, A3>::registerAs(#Function);

/*---------------------------- Four arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_6(Ret, Function, A1, A2, A3, A4)                 \
  template <>                                                                  \
  const vrpc::GlobalFunctionRegistrar<                                         \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Function)), &Function,     \
      Ret, A1, A2, A3, A4>                                                     \
      vrpc::RegisterGlobalFunction<                                            \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Function)), &Function, \
          Ret, A1, A2, A3, A4>::registerAs(#Function);

/*---------------------------- Five arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_7(Ret, Function, A1, A2, A3, A4, A5)             \
  template <>                                                                  \
  const vrpc::GlobalFunctionRegistrar<                                         \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Function)), &Function, \
      Ret, A1, A2, A3, A4, A5>                                                 \
      vrpc::RegisterGlobalFunction<                                            \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Function)),        \
          &Function, Ret, A1, A2, A3, A4, A5>::registerAs(#Function);

/*----------------------------- Six arguments --------------------------------*/

// global
#define _VRPC_GLOBAL_FUNCTION_8(Ret, Function, A1, A2, A3, A4, A5, A6)      \
  template <>                                                               \
  const vrpc::GlobalFunctionRegistrar<                                      \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(Function)),     \
      &Function, Ret, A1, A2, A3, A4, A5, A6>                               \
      vrpc::RegisterGlobalFunction<                                         \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(Function)), \
          &Function, Ret, A1, A2, A3, A4, A5, A6>::registerAs(#Function);

#endif
