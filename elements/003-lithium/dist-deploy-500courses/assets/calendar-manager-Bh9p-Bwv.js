import{d as e}from"./index-DmdMIfaA.js";var t=class{constructor(e,t){this.app=e,this.container=t,this.elements={}}async init(){await this.render(),this.setupEventListeners()}async render(){this.container.innerHTML=`
      <div class="calendar-manager-container">
        <div class="placeholder-header">
          <fa fa-calendar-days></fa>
          <h2>Calendar Manager</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle calendar events and scheduling.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Calendar Manager module is under development.</span>
          </div>
        </div>
      </div>
    `,e(this.container)}setupEventListeners(){}teardown(){}};export{t as default};