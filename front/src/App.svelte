<script>
import { fade } from "svelte/transition";
import CharacterSelect from "./lib/CharacterSelect.svelte";
import { display_error, ERROR_DISPLAY } from "./error_display";
import { BACKEND_URL } from "./consts";

let num_selected = 0;

async function launch() {
  if (num_selected > 0) {
    let res = await fetch(`${BACKEND_URL}/launch`, {method: "POST", body: JSON.stringify({"num_clients": num_selected})});
    const j = await res.json();
    if (!res.ok) {
      display_error(j.details)
    }
  }
}

</script>

<main>
<h1>wowibotti_suite: Injector :D</h1>
{#await fetch(`${BACKEND_URL}/config`).then(async r => {
  const j = await r.json();
  if (!r.ok) {
    throw new Error(`Fetching config failed: HTTP ${r.status}: ${j.details}`);
  } else {
    return Promise.resolve(j)
  }})
}
<div>Loading config...</div>
{:then {result}} 
<CharacterSelect config={result} bind:num_selected={num_selected}/>
<div>
  <button disabled={num_selected < 1} on:click={launch}>Launch {num_selected} client(s)</button>
</div>
{#if $ERROR_DISPLAY !== undefined}
  <div class="error-display" transition:fade>
    {#if $ERROR_DISPLAY.error}
    <p>{$ERROR_DISPLAY.error}</p>
    {/if}
  </div>
{/if}
{:catch error}
<div>{error}</div>
{/await}
</main>

<style>

</style>
