<script>
import { BACKEND_URL } from "../consts";
import Character from "./Character.svelte";
import { display_error } from "../error_display";
    import { fade, slide } from "svelte/transition";

export let config;
export let num_selected = 0;

let injection = {
  show_result: false,
  clients: [],
  timeout: undefined,
}; 

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

function checkbox_onchange(e) {
    if (e.target.checked) {
        num_selected += 1;
    }
    else {
        num_selected -= 1;
    }
}

</script>

<div class="form-container">
  <form on:submit|preventDefault={submit_inject_form}>
    <div class="characters">
    {#each config.characters as c}
    <Character character={c} onchange={checkbox_onchange}/>
    {/each}
    </div>
    <input type="submit" value="Inject">
  </form>
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

<style>
.form-container {
  display: flex;
  flex-direction: column;
  margin-bottom: 1em;
  line-height: 1.1;
}

.characters {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  margin-bottom: 2em;
}

.injection-result {
  margin-bottom: 1em;
}
</style>