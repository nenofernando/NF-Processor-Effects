// Regression tests for the in-plugin "Check for updates": version comparison and the
// download-URL allow-list. No network access.
#include "../Source/License/NFUpdateCheck.h"
#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(cond) do { if (! (cond)) { std::printf ("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)

// Opt-in network check against the real server: NFPEUpdateTests --live
static void liveChecks()
{
    // an old version must be told an update exists; the version the server returns must then be "up to date"
    auto old = nfupdate::fetchLatest ("nf-processor-effects", "0.0.1");
    CHECK (old.reachedServer);
    CHECK (old.updateAvailable);
    CHECK (nfupdate::isTrustedDownloadUrl (old.downloadUrl));
    std::printf ("live: latest nf-processor-effects = %s, url = %s\n", old.latestVersion.toRawUTF8(), old.downloadUrl.toRawUTF8());

    auto same = nfupdate::fetchLatest ("nf-processor-effects", old.latestVersion);
    CHECK (same.reachedServer);
    CHECK (! same.updateAvailable);

    auto missing = nfupdate::fetchLatest ("produto-que-nao-existe", "1.0.0");
    CHECK (! missing.reachedServer);
    CHECK (missing.error == "product_not_found");
    std::printf ("live: unknown product -> %s\n", missing.error.toRawUTF8());
}

int main (int argc, char** argv)
{
    using nfupdate::compareVersions;

    CHECK (compareVersions ("1.0.1", "1.0.0") > 0);
    CHECK (compareVersions ("1.0.0", "1.0.1") < 0);
    CHECK (compareVersions ("1.0.0", "1.0.0") == 0);
    CHECK (compareVersions ("1.0", "1.0.0") == 0);          // missing part counts as 0
    CHECK (compareVersions ("1.10.0", "1.9.0") > 0);        // numeric, not alphabetical
    CHECK (compareVersions ("2.0.0", "1.99.99") > 0);
    CHECK (compareVersions ("1.0.0.1", "1.0.0") > 0);

    CHECK (nfupdate::isTrustedDownloadUrl ("https://plugins.masterlibrary.com.br/api/download?product=nf-processor-effects&os=macos"));
    CHECK (! nfupdate::isTrustedDownloadUrl ("http://plugins.masterlibrary.com.br/api/download"));          // no plain http
    CHECK (! nfupdate::isTrustedDownloadUrl ("https://plugins.masterlibrary.com.br.evil.example/x"));      // look-alike host
    CHECK (! nfupdate::isTrustedDownloadUrl ("https://evil.example/api/download"));
    CHECK (! nfupdate::isTrustedDownloadUrl (""));

    if (argc > 1 && std::strcmp (argv[1], "--live") == 0)
        liveChecks();

    if (failures == 0)
        std::printf ("NFUpdateCheck tests: all passed\n");
    return failures == 0 ? 0 : 1;
}
