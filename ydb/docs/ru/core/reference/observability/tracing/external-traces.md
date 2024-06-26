# Передача внешнего trace-id в {{ ydb-short-name }}

## gRPC API

{{ ydb-short-name }} поддерживает передачу внешних trace-id для построения цельной трассы операции. Передача trace-id производится согласно [спецификации trace context](https://w3c.github.io/trace-context/#traceparent-header) — через заголовок `traceparent` gRPC запроса должна передаваться строка вида `00-0af7651916cd43dd8448eb211c80319c-b7ad6b7169203331-01`. Она состоит из 4 частей, разделенных символом `-`:

1. Версия – на момент написания документации спецификация определяет только версию 00.
1. Trace id – идентификатор трассы, частью которой является запрос.
1. Parent id – идентификатор родительского спана.
1. Флаги – набор рекомендаций по работе с переданными в заголовке данными.

Только версия 00 поддерживается, флаги игнорируются. Если заголовок не соответствует спецификации, он игнорируется. Все получаемые таким образом трассы имеют [уровень детализации](./setup.md#tracing-levels) 13 (все компоненты трассируются с уровнем `Detailed`).

{% note warning %}

При наличии секции [`external_throttling`](./setup.md#external-throttling) и потоке запросов, превышающем установленные лимиты, могут трассироваться не все запросы. При отсутствии секции [`external_throttling`](./setup.md#external-throttling) заголовок `traceparent` **игноруется** и никакие внешние трассы не продолжаются.

{% endnote %}

## Поддержка SDK

Некоторые SDK поддерживают передачу trace-id, их список, а так же примеры использования вы можете найти на странице [{#T}](../../../reference/ydb-sdk/recipes/debug-jaeger.md).
