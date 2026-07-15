import{d as e}from"./index-DmdMIfaA.js";var t=class{constructor(e,t){this.app=e,this.container=t,this.elements={}}async init(){await this.render(),this.setupEventListeners()}async render(){this.container.innerHTML=`
      <div class="ticketing-manager-container">
        <div class="placeholder-header">
          <fa fa-ticket></fa>
          <h2>Ticketing Manager</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle support tickets and issue tracking.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Ticketing Manager module is under development.</span>
          </div>
        </div>
      </div>
    `,e(this.container)}setupEventListeners(){}teardown(){}};export{t as default};