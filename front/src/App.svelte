<script>
import { fade } from "svelte/transition";
import CharacterSelect from "./lib/CharacterSelect.svelte";
import { display_error, ERROR_DISPLAY } from "./error_display";
import { BACKEND_URL } from "./consts";
    import PatchOption from "./lib/PatchOption.svelte";

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

let injection = {
  show_result: false,
  clients: [],
  timeout: undefined,
}; 

async function submit_inject_form(e) {
  const query = {
    enabled_characters: [],
    enabled_patches: [],
  }
  for (const [k, v] of new FormData(e.target).entries()) {
    if (k.startsWith('character-') && v === 'on') query.enabled_characters.push(k.slice('character-'.length));
    if (k.startsWith('patch-') && v === 'on') query.enabled_patches.push(k.slice('patch-'.length));
  }
  let res = await fetch(`${BACKEND_URL}/inject`, {
    method: "POST",
    body: JSON.stringify(query),
  });
  let j = await res.json();
  if (!res.ok) {
    display_error({error: `injection failed: ${j.details}`})
  }
  else {
    if (injection.timeout) {
      clearTimeout(injection.timeout);
    }
    injection = {
      clients: j.clients,
      timeout: setTimeout(() => injection = {clients: [], timeout: undefined, show_result: false}, 5000),
      show_result: true,
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
<div class="form-container">
  <form on:submit|preventDefault={submit_inject_form}>
  <CharacterSelect config={result} bind:num_selected={num_selected}/>
  <h4>Select patches</h4>
  <div class="patch-select">
    <div></div>
    <input type="checkbox" disabled checked>
    <div class="text-align-left"><span class="monospace">EndScene_hook</span> <i>(Always enabled)</i></div>
    {#each result.available_patches as p}
    <PatchOption config={p}/>
    {/each}
  </div>
  <input type="submit" value="Inject">
  </form>
</div>
<div>
  <button disabled={num_selected < 1} on:click={launch}>Launch {num_selected} client(s)</button>
</div>
{#if injection.show_result }
<div class="injection-result" transition:fade>
  {#if injection.clients.length > 0}
  <h3>Successfully injected dll to:</h3>
  {#each injection.clients as c}
    <p>Pid: {c.pid}</p>
  {/each}
  {:else}
    <p>No clients were injected!</p>
  {/if}
</div>
{/if}

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
.form-container {
  display: flex;
  flex-direction: column;
  margin-bottom: 1em;
  line-height: 1.1;
}

.injection-result {
  margin-bottom: 1em;
}

.patch-select {
  display: grid;
  grid-template-columns: 0.5fr 0.1fr 1fr;
  margin-bottom: 2em;
  grid-row-gap: 0.5em;
}

:global(.text-align-left) {
  text-align: left;
}

:global(.monospace) {
    font-family: monospace;
}

</style>
