<script>
import { BACKEND_URL } from "../consts";
import Character from "./Character.svelte";
import { display_error } from "../error_display";

export let config;
export let num_selected = 0;

let injection_result = undefined; 

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
    injection_result = j
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



<style>
.form-container {
  display: flex;
  flex-direction: column;
  margin-bottom: 2em;
  line-height: 1.1;
}

.characters {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  margin-bottom: 2em;
}

</style>