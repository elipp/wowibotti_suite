<script>
    import { fade, slide } from "svelte/transition";
import Character from "./lib/Character.svelte";
const BACKEND_URL = "http://127.0.0.1:7070";

let injection_result = undefined; 
let result_log_timeout = undefined;

async function submit_inject_form(e) {
  let enabled_characters = []
  for (const [k, v] of new FormData(e.target).entries()) {
    if (v === 'on') enabled_characters.push(k)
  }
  let res = await fetch(`${BACKEND_URL}/inject`, {
    method: "POST",
    body: JSON.stringify({
      enabled_characters,
    }),
  });
  let j = await res.json();
  if (!res.ok) {
    injection_result = {error: `injection failed: ${j.details}`}
  }
  else {
    injection_result = j
  }
  if (result_log_timeout !== undefined) {
    clearTimeout(result_log_timeout);
  }
  result_log_timeout = setTimeout(() => { injection_result = undefined }, 5000)
}

</script>

<main>
<h1>Wowiboddihookdll injector</h1>
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
    <div class="characters">
    {#each result.characters as c}
    <Character character={c}/>
    {/each}
    </div>
    <input type="submit" value="Inject">
  </form>
  {#if injection_result !== undefined}
    <div class="injection-result" transition:fade>
      {#if injection_result.error}
      <p>{injection_result.error}</p>
      {:else if injection_result.clients.length > 0}
        <h3>Successfully injected dll to clients:</h3>
        {#each injection_result.clients as c}
        <p>Pid: {c.pid}</p>
        {/each}
      {:else}
        <p>No clients found!</p>
      {/if}
    </div>
  {/if}
</div>
{:catch error}
<div>{error}</div>
{/await}
</main>

<style>

.form-container {
  display: flex;
  flex-direction: column;
}

.characters {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  margin-bottom: 2em;
}
</style>
