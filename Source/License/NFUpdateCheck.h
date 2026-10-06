#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <thread>

/** "Procurar atualização" dos plugins NF.

    O plugin pergunta ao painel qual é a versão mais nova do produto (só quando
    o cliente clica no menu - nada roda ao abrir o plugin nem em loop) e compara
    com a própria versão. Nada do usuário é enviado: só a tag do produto e o
    sistema (windows/macos).

    Uso (no handler do item de menu, com o editor protegido por SafePointer):

        nfupdate::checkAsync ("nf-meu-plugin", JucePlugin_VersionString, [safeThis] (nfupdate::Result r)
        {
            if (safeThis == nullptr) return;   // o editor fechou enquanto a consulta rodava
            ...
        });

    `checkAsync` só captura VALORES e nunca toca em componente: quem chama é
    que protege o próprio ciclo de vida dentro do callback (mesmo cuidado do
    use-after-free corrigido no NFLicenseManager). */
namespace nfupdate
{
    inline const juce::String& endpoint()
    {
        static const juce::String url ("https://plugins.masterlibrary.com.br/api/latest-version");
        return url;
    }

    /** Só abre links que apontam pro nosso próprio domínio, mesmo que a resposta venha adulterada. */
    inline bool isTrustedDownloadUrl (const juce::String& url)
    {
        return url.startsWith ("https://plugins.masterlibrary.com.br/");
    }

    struct Result
    {
        bool reachedServer = false;      // false = sem internet, resposta inválida ou produto sem versão publicada
        bool updateAvailable = false;
        juce::String latestVersion;
        juce::String downloadUrl;
        juce::String error;
    };

    /** Compara versões "1.0.2" numericamente; parte ausente vale 0 ("1.0" == "1.0.0").
        < 0 se a < b, 0 se iguais, > 0 se a > b. */
    inline int compareVersions (const juce::String& a, const juce::String& b)
    {
        const auto pa = juce::StringArray::fromTokens (a, ".", "");
        const auto pb = juce::StringArray::fromTokens (b, ".", "");
        const int n = juce::jmax (pa.size(), pb.size());

        for (int i = 0; i < n; ++i)
        {
            const int x = i < pa.size() ? pa[i].getIntValue() : 0;
            const int y = i < pb.size() ? pb[i].getIntValue() : 0;
            if (x != y)
                return x < y ? -1 : 1;
        }
        return 0;
    }

    /** Bloqueante (rede): chamar FORA da message thread. */
    inline Result fetchLatest (const juce::String& productTag, const juce::String& currentVersion)
    {
        Result r;

       #if JUCE_WINDOWS
        const juce::String os = "windows";
       #elif JUCE_MAC
        const juce::String os = "macos";
       #else
        r.error = "unsupported_os";
        return r;
       #endif

        auto url = juce::URL (endpoint()).withParameter ("product", productTag).withParameter ("os", os);

        int statusCode = 0;
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (8000)
                           .withStatusCode (&statusCode);

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
        {
            r.error = "connection_failed";
            return r;
        }

        const auto json = juce::JSON::parse (stream->readEntireStreamAsString());

        if (statusCode != 200)
        {
            r.error = json.getProperty ("error", "http_error").toString();
            return r;
        }

        r.latestVersion = json.getProperty ("version", "").toString();
        r.downloadUrl = json.getProperty ("download_url", "").toString();

        if (r.latestVersion.isEmpty() || ! isTrustedDownloadUrl (r.downloadUrl))
        {
            r.error = "invalid_response";
            return r;
        }

        r.reachedServer = true;
        r.updateAvailable = compareVersions (r.latestVersion, currentVersion) > 0;
        return r;
    }

    /** Faz a consulta numa thread própria e chama `onDone` na message thread. */
    inline void checkAsync (juce::String productTag, juce::String currentVersion, std::function<void (Result)> onDone)
    {
        std::thread ([productTag, currentVersion, onDone = std::move (onDone)]
        {
            auto result = fetchLatest (productTag, currentVersion);

            juce::MessageManager::callAsync ([result, onDone]
            {
                if (onDone)
                    onDone (result);
            });
        }).detach();
    }
}
