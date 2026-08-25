/**
 * SNAP Live Cloud API Multi-Scenario Tester (Node.js / JavaScript)
 * Base URL: https://snap-api-673324870645.asia-northeast3.run.app
 */

const BASE_URL = "https://snap-api-673324870645.asia-northeast3.run.app";

async function requestJson(endpoint, payload = null) {
    const url = `${BASE_URL}${endpoint}`;
    const options = {
        method: payload ? "POST" : "GET",
        headers: {
            "User-Agent": "SNAP-Client-NodeJS/1.0",
            ...(payload ? { "Content-Type": "application/json" } : {})
        },
        ...(payload ? { body: JSON.stringify(payload) } : {})
    };

    const t0 = performance.now();
    const resp = await fetch(url, options);
    const duration = performance.now() - t0;
    const data = await resp.json();
    return { data, duration, serverLat: resp.headers.get("X-Response-Time") };
}

async function main() {
    console.log("=".repeat(80));
    console.log("🚀 SNAP Cloud REST API Live Tester (JavaScript / Node.js)");
    console.log(`🔗 Target Base URL: ${BASE_URL}`);
    console.log("=".repeat(80));

    // 1. Health Check
    console.log("\n[1] Checking Service Health (`GET /v1/health`)...");
    const health = await requestJson("/v1/health");
    console.log(`Status  : ${health.data.status}`);
    console.log(`Version : ${health.data.version}`);
    console.log(`Engine  : ${health.data.engine_loaded ? "Loaded" : "Offline"}`);
    console.log(`Active Languages: ${health.data.active_languages}`);
    console.log(`⚡ Client RTT: ${health.duration.toFixed(2)}ms`);

    // 2. Single Sentence Normalization
    console.log("\n[2] Single Sentence Normalization (`POST /v1/normalize`)...");
    const norm = await requestJson("/v1/normalize", {
        text: "여기서 3번 버스를 타고 3번 갈아타세요. ChatGPT와 LG CNS를 사용합니다.",
        custom_dict: {
            "ChatGPT": "챗지피티",
            "LG CNS": "엘지씨엔에스"
        },
        config: {
            prosody_format: "tags",
            return_ipa: true
        }
    });
    console.log(`Original Text   : ${norm.data.data.original_text}`);
    console.log(`Normalized Text : ${norm.data.data.normalized_text}`);
    console.log(`Phonemes (G2P)  : ${norm.data.data.phonemes}`);
    console.log(`IPA Phonetics   : ${norm.data.data.ipa}`);
    console.log(`⚡ Engine Latency: ${norm.data.meta.latency_ms}ms (RTT: ${norm.duration.toFixed(2)}ms)`);

    console.log("\n" + "=".repeat(80));
    console.log("✅ Node.js SNAP API Live Test Completed Successfully!");
    console.log("=".repeat(80));
}

main().catch(console.error);
