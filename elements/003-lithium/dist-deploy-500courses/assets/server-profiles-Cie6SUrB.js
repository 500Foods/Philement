import{d as e}from"./index-DmdMIfaA.js";var t=class{constructor(e,t){this.app=e,this.container=t,this.elements={}}async init(){await this.render(),this.setupEventListeners()}async render(){this.container.innerHTML=`
      <div class="user-profiles-container">
        <div class="placeholder-header">
          <fa fa-users></fa>
          <h2>Profile Manager</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle user profile administration and management.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Profile Manager module is under development.</span>
          </div>
        </div>
      </div>
    `,e(this.container)}setupEventListeners(){}teardown(){}};export{t as default};