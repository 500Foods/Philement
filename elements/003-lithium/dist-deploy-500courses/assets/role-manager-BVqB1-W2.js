import{d as e}from"./index-DmdMIfaA.js";var t=class{constructor(e,t){this.app=e,this.container=t,this.elements={}}async init(){await this.render(),this.setupEventListeners()}async render(){this.container.innerHTML=`
      <div class="role-manager-container">
        <div class="placeholder-header">
          <fa fa-user-shield></fa>
          <h2>Role Manager</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle user roles and permission assignments.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Role Manager module is under development.</span>
          </div>
        </div>
      </div>
    `,e(this.container)}setupEventListeners(){}teardown(){}};export{t as default};