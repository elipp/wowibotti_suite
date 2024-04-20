<script>
import { fade } from "svelte/transition";
import CharacterSelect from "./lib/CharacterSelect.svelte";
import { display_error, ERROR_DISPLAY } from "./error_display";
const BACKEND_URL = "http://127.0.0.1:7070";

let num_selected = 0;
$: console.debug($ERROR_DISPLAY)

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
  <button on:click={launch}>Launch {num_selected} client(s)</button>
</div>
{#if $ERROR_DISPLAY !== undefined}
  <div class="error-display" transition:fade>
    {#if $ERROR_DISPLAY.error}
    <p>{$ERROR_DISPLAY.error}</p>
    {:else if $ERROR_DISPLAY.clients.length > 0}
      <h3>Successfully injected dll to clients:</h3>
      {#each $ERROR_DISPLAY.clients as c}
      <p>Pid: {c.pid}</p>
      {/each}
    {:else}
      <p>No clients found!</p>
    {/if}
  </div>
{/if}
{:catch error}
<div>{error}</div>
{/await}
</main>

<style>

</style>
